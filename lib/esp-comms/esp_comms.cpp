#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include "lwip/apps/sntp.h"
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>
#include <FS.h>
#include "json_settings.h"
#include "web_console.h"
#include "esp_comms.h"

AsyncWebServer server(80);
AsyncWebSocket g_ws("/ws"); // access at ws://[esp ip]/ws

static AwsEventHandler ws_data_cb = NULL;

// Only one task can send data on a web-socket, the others have to wait.
// static SemaphoreHandle_t ws_mutex = NULL;

// -----------------
//  WS server stuff
// -----------------
// wrapper to filter out just the received data and call ws_data_cb with it
void _ws_event(
	AsyncWebSocket * server,
	AsyncWebSocketClient * client,
	AwsEventType type,
	void * arg,
	uint8_t *data,
	size_t len
) {
	//Handle WebSocket event
	AwsFrameInfo * info;

	switch (type) {
		case WS_EVT_CONNECT:
			//client connected
			log_d("ws[%u] connect", client->id());
			client->printf(
				"{\"hello\": \"%s\"}",
				jGetS(getSettings(), "hostname", WIFI_HOST_NAME)
			);
			break;

		case WS_EVT_DISCONNECT:
			wsDisableLog();
			log_d("ws[%u] disconnect: %u", client->id(), client->id());
			break;

		case WS_EVT_ERROR:
			// error was received from the other end
			log_e("ws[%u] error(%u): %s", client->id(), *((uint16_t*)arg), (char*)data);
			break;

		case WS_EVT_PONG:
			log_d("ws[%u] pong[%u]: %s", client->id(), len, (len)?(char*)data:"");
			break;

		case WS_EVT_DATA:
			// Careful with logging, data might not be zero terminated!
			// log_v("ws[%u] data[%u]: %s", client->id(), len, (len)?(char*)data:"");

			info = (AwsFrameInfo*)arg;
			if (!info->final || info->index != 0 || info->len != len) {
				log_e("not supporting RX of multi-part message");
				return;
			}

			// the whole message is in a single frame and we got it all
			if (len <= 0)
				return;

			if (ws_data_cb)
				ws_data_cb(server, client, type, arg, data, len);
			break;

		default:
			log_e("ws[%u] unhandled: %u", client->id(), type);
	}
}

// --------------------------
//  WIFI & http server stuff
// --------------------------
static bool isOta = false;

void init_comms(fs::FS &serveFs, const char* servePath, AwsEventHandler on_ws_data)
{
	// register ws data RX handler
	ws_data_cb = on_ws_data;

	cJSON *s = getSettings();

	// Where do the connection settings come from?
	//   * take `wifi_ssid` and `wifi_pw` keys in the .json config
	//   * if that fails, if falls back to the build flags in platformio.ini
	//   * if it fails to connect it switches to AP mode
	//   * AP mode can be enforced by pushing the `FLASH` button while booting
	const char *ssid = jGetS(s, "wifi_ssid", WIFI_NAME);
	const char *host = jGetS(s, "hostname", WIFI_HOST_NAME);

	// ------------------------------
	//  connect to WIFI
	// ------------------------------
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
			delay(000);
		}
	} else {
		WiFi.disconnect(true, true);
		WiFi.softAPsetHostname(host);
		WiFi.softAP(host);
		log_e(
			"No STA connection, switching to AP mode (%s)",
			WiFi.softAPIP().toString().c_str()
		);
	}

	// -------------------------------
	//  Over The Air firmware updates
	// -------------------------------
	isOta = jGetB(s, "enable_ota", false);
	if (isOta) {
		ArduinoOTA.setHostname(host);
		ArduinoOTA.begin();
	} else {
		MDNS.begin(host);
		MDNS.enableArduino(80);
	}

	// -------------------------------
	//  static http server / WS
	// -------------------------------
	// attach AsyncWebSocket
	g_ws.onEvent(_ws_event);
	server.addHandler(&g_ws);

	// attach filesystem root at URL /fs
	server.serveStatic("/", serveFs, servePath);
	server.begin();

	// ------------------------------
	//  Set the clock / print time
	// ------------------------------
	// Set timezone to Eastern Standard Time and print local time
	const char *tz_str = jGetS(getSettings(), "timezone", "PST8PDT");
	log_i("Setting timezone to TZ = %s", tz_str);
	setenv("TZ", tz_str, 1);
	tzset();
	time_t now = time(NULL);
	struct tm timeinfo = {0};
	char strftime_buf[64];
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	log_i("Local Time: %s (%ld)", strftime_buf, now);
}
