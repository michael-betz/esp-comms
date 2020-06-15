// demonstrates the use of esp-comms
#include <stdio.h>
#include "Arduino.h"
#include "WiFi.h"
#include "SPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/rtc.h"
#include "esp_comms.h"
#include "web_console.h"
#include "json_settings.h"
#include "esp_heap_caps.h"

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
	// report initial status
	log_w(
		"reset reason: %d, heap: %d, min_heap: %d",
		rtc_get_reset_reason(0),
		esp_get_free_heap_size(),
		esp_get_minimum_free_heap_size()
	);

	// forwards each serial character to web-console
	web_console_init();
	log_w(
		"after web_console, heap: %d, min_heap: %d",
		esp_get_free_heap_size(),
		esp_get_minimum_free_heap_size()
	);

	SPIFFS.begin(true, "/spiffs", 5);
	log_w(
		"after SPIFFS, heap: %d, min_heap: %d",
		esp_get_free_heap_size(),
		esp_get_minimum_free_heap_size()
	);

	// When settings.json cannot be opened, it will copy the default_settings over
	set_settings_file("/spiffs/settings.json", "/spiffs/default_settings.json");
	log_w(
		"after settings, heap: %d, min_heap: %d",
		esp_get_free_heap_size(),
		esp_get_minimum_free_heap_size()
	);

	init_comms(SPIFFS, "/", on_ws_data);
	log_w(
		"after init_comms, heap: %d, min_heap: %d",
		esp_get_free_heap_size(),
		esp_get_minimum_free_heap_size()
	);

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

	if (cycle % 100 == 0) {
		log_i(
			"%.1f degC, stack: %d, heap(): %d / %d",
			temperatureRead() - 17,
			uxTaskGetStackHighWaterMark(NULL),
			esp_get_free_heap_size(),
			esp_get_minimum_free_heap_size()
		);
	}

 	g_ws.cleanupClients();

	cycle++;
	delay(100);
}
