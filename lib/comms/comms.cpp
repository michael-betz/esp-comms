#include <WiFi.h>
#include <WebServer.h>
#include "detail/mimetable.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "lwip/apps/sntp.h"
#include <ArduinoWebsockets.h>
#include <SPIFFS.h>
#include <FS.h>
#include "json_settings.h"
#include "web_console.h"
#include "comms.h"

using namespace websockets;

WebsocketsClient ws_client;
WebsocketsClient *g_ws_client = &ws_client;
static WebsocketsServer ws_server;

// http server
static WebServer http_server(80);

// -----------------
//  WS server stuff
// -----------------
static void onEventCallback(WebsocketsEvent event, String data)
{
	if(event == WebsocketsEvent::ConnectionOpened) {
		log_d("<open>");	// never called on server, bug? :(
	} else if(event == WebsocketsEvent::ConnectionClosed) {
		wsDisableLog();
		g_ws_client = NULL;
		log_d("<close>");
	} else if(event == WebsocketsEvent::GotPing) {
		log_d("<ping>");
	} else if(event == WebsocketsEvent::GotPong) {
		log_d("<pong>");
	}
}

static void onMessageCallback(WebsocketsMessage message)
{
	log_d("%s", message.data());
}

static void (*ws_msg_callback)(WebsocketsMessage) = onMessageCallback;

// --------------------------
//  WIFI & http server stuff
// --------------------------
static bool isOta = false;
void refresh_comms(void)
{
	if (ws_server.poll() && !ws_client.available()) {
		ws_client = ws_server.accept();
		g_ws_client = &ws_client;
		ws_client.onMessage(ws_msg_callback);
		ws_client.onEvent(onEventCallback);
		// Send hello message to JS
		String host = String(jGetS(getSettings(), "hostname", WIFI_HOST_NAME));
		ws_client.send("{\"hello\": \"" + host + "\"}");
	}

	if (ws_client.available())
		ws_client.poll();

	if (isOta)
		ArduinoOTA.handle();

	http_server.handleClient();
}

static void comms_task(void *pvParameters)
{
	while(true) {
		refresh_comms();
		delay(30);
	}
}

TaskHandle_t t_comms = NULL;
void init_comms(bool createCommsTask, fs::FS &serveFs, const char* servePath, void (*msg_callback)(WebsocketsMessage))
{
	if (msg_callback)
		ws_msg_callback = msg_callback;

	cJSON *s = getSettings();

	// Where do the connection settings come from?
	//   * take `wifi_ssid` and `wifi_pw` keys in the .json config
	//   * if that fails, if falls back to the build flags in platformio.ini
	//   * if it fails to connect it switches to AP mode
	//   * AP mode can be enforced by pushing the `FLASH` button while booting
	const char *ssid = jGetS(s, "wifi_ssid", WIFI_NAME);
	const char *host = jGetS(s, "hostname", WIFI_HOST_NAME);

	WiFi.setHostname(host);
	WiFi.begin(ssid, jGetS(s, "wifi_pw", WIFI_PW));
	log_i("This is %s, connecting to %s", host, ssid);

	for (int i=0; i<=100; i++) {
		if (WiFi.status() == WL_CONNECTED || !digitalRead(0)) {
			break;
		}
		delay(100);
	}
	if (WiFi.status() == WL_CONNECTED) {
		if (!sntp_enabled()) {
			log_i("Initializing SNTP");
			sntp_setoperatingmode(SNTP_OPMODE_POLL);
			sntp_setservername(0, (char *)"pool.ntp.org");
			sntp_init();
		}
	} else {
		WiFi.disconnect();
		WiFi.softAPsetHostname(host);
		WiFi.softAP(host);
		log_e(
			"No STA connection, switching to AP mode (%s)",
			WiFi.softAPIP().toString().c_str()
		);

	}

	isOta = jGetB(s, "enable_ota", false);
	if (isOta) {
		ArduinoOTA.setHostname(host);
		ArduinoOTA.begin();
	} else {
		MDNS.begin(host);
	}

	http_server.serveStatic("/", serveFs, servePath, "");
	http_server.begin();

	ws_server.listen(8080);
	log_i("Websocket at 8080 online: %d", ws_server.available());

	if (createCommsTask) {
		xTaskCreateUniversal(comms_task, "comms", 4000, NULL, 1, &t_comms, -1);
	}
}
