// demonstrates the use of esp-comms
#include <stdio.h>
#include "Arduino.h"
#include "ArduinoWebsockets.h"
#include "SPIFFS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_comms.h"
#include "web_console.h"
#include "json_settings.h"

// Web socket RX callback
static void onMsg(websockets::WebsocketsMessage msg)
{
	if (msg.length() <= 0)
		return;

	switch (msg.c_str()[0]) {
		case 'a':
			wsDumpRtc();  // read rolling log buffer in RTC memory
			break;
		case 'b':
			settings_ws_handler(msg);  // read / write settings.json
			break;
		case 'r':
			ESP.restart();
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

	init_comms(false, SPIFFS, "/", onMsg);

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
			"%.1f degC, stack loop(): %d,  comms(): %d",
			temperatureRead() - 17,
			uxTaskGetStackHighWaterMark(NULL),
			uxTaskGetStackHighWaterMark(t_comms)
		);

	refresh_comms();  // only needed with init_comms(false, ...)

	cycle++;
	delay(100);
}
