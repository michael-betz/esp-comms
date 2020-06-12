// demonstrates the use of esp-comms
#include <stdio.h>
#include "Arduino.h"
#include "WiFi.h"
#include "SPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_comms.h"
#include "web_console.h"
#include "json_settings.h"

// Web socket RX data received callback
static void on_ws_data(
	AsyncWebSocket * server,
	AsyncWebSocketClient * client,
	AwsEventType type,
	void * arg,
	uint8_t *data,
	size_t len
) {
	switch (data[0]) {
		case 'a':
			wsDumpRtc(client);  // read rolling log buffer in RTC memory
			break;

		case 'b':
			settings_ws_handler(client, data, len);  // read / write settings.json
			break;

		case 'r':
			ESP.restart();
			break;

		case 'h':
			client->printf(
				"h{\"heap\": %d, \"min_heap\": %d}",
				ESP.getFreeHeap(), ESP.getMinFreeHeap()
			);
			break;
	}
}

void setup()
{
	// forwards each serial character to web-console
	web_console_init();

	SPIFFS.begin(true, "/spiffs", 5);

	// When settings.json cannot be opened, it will copy the default_settings over
	set_settings_file("/spiffs/settings.json", "/spiffs/default_settings.json");

	init_comms(SPIFFS, "/", on_ws_data);

	// This is how to access a string item from settings.json
	log_d(
		"settings[hostname] = %s",
		jGetS(getSettings(), "hostname", "fallback_value")
	);

	// Demonstrate logging levels (colored!)
	log_v("VERBOSE");
	log_d("DEBUG");
	log_i("INFO");
	log_w("WARNING");
	log_e("ERROR");
}

void loop() {
	static unsigned cycle=0;

	// ESP-IDF style log. The TAG will be ignored :(
	if (cycle % 300 == 0)
		ESP_LOGW("BLA", "wouush %d!!!", cycle);

	// Arduino style log
	if (cycle % 700 == 0)
		log_e("FIZZZ %d!!!", cycle);

	if (cycle % 100 == 0)
		log_i(
			"%.1f degC, stack loop(): %d, heap(): %d, %d",
			temperatureRead() - 17,
			uxTaskGetStackHighWaterMark(NULL),
			ESP.getFreeHeap(),
			ESP.getMinFreeHeap()
		);

 	g_ws.cleanupClients();

	cycle++;
	delay(100);
}
