#ifndef ESP_COMMS_H
#define ESP_COMMS_H

#include <FS.h>
#include "ESPAsyncWebServer.h"

extern AsyncWebSocket g_ws;

extern TaskHandle_t t_comms;

// needs to be called once
// with init_comms(false) you'll need to call refresh_comms() regularly
// init_comms(true) is buggy
void init_comms(fs::FS &serveFs, const char* servePath, AwsEventHandler on_ws_data);

// needs to be called regularly in non-threaded mode
// void refresh_comms(void);

// write data to open websocket. Thread-safe. Returns true on success.
// bool write_ws(const char *data, unsigned len);

#endif
