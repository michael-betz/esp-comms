#ifndef WCONSOLE_H
#define WCONSOLE_H
#include <ArduinoWebsockets.h>

// Rolling buffer size in RTC mem for log entries in bytes
#define LOG_FILE_SIZE 3576

// add one character to the RTC log buffer.
// Once a full line is accumulated, push it to the WS.
// to forward all UART data, register it during init with:
// ets_install_putc2(wsDebugPutc);
void wsDebugPutc(char c);

// dump the whole RTC buffer to the WS, oldest entries first.
// call this once the WS connection is open.
void wsDumpRtc(void);

void wsDisableLog();

void web_console_init();

#endif
