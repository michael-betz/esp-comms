#ifndef COMMS_H
#define COMMS_H

#include <ArduinoWebsockets.h>
#include <FS.h>

extern TaskHandle_t t_comms;
extern websockets::WebsocketsClient *g_ws_client;

// needs to be called once
// with init_comms(false) you'll need to call refresh_comms() regularly
// init_comms(true) is buggy
void init_comms(bool createCommsTask, fs::FS &serveFs, const char* servePath, void (*msg_callback)(websockets::WebsocketsMessage));

// needs to be called regularly in non-threaded mode
void refresh_comms(void);

// write data to currently open websocket. Thread-safe.
// void write_ws(const char *data, unsigned len);

#endif
