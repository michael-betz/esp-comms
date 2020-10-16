#ifndef ESP_COMMS_H
#define ESP_COMMS_H

#include <FS.h>
#include "ESPAsyncWebServer.h"

extern AsyncWebSocket g_ws;

// needs to be called once
void init_comms(fs::FS &serveFs, const char* servePath, AwsEventHandler on_ws_data);

void startServices(fs::FS &serveFs, const char* servePath, AwsEventHandler on_ws_data);

void stopServices();

#endif
