#include <stdio.h>
#include <stdarg.h>
#include "esp_attr.h"
#include "rom/rtc.h"
#include "esp_comms.h"
#include "web_console.h"

// put the buffer and write pointer in RTC memory,
// such that it survives sleep mode
RTC_NOINIT_ATTR static char rtcLogBuffer[LOG_FILE_SIZE];
RTC_NOINIT_ATTR static char *rtcLogWritePtr = rtcLogBuffer;

static const char *logBuffEnd = rtcLogBuffer + LOG_FILE_SIZE - 1;

static bool isLog = false;
AsyncWebSocketClient *wsc = NULL;

void wsDisableLog()
{
	isLog = false;
}

void wsDebugPutc(char c)
{
	// ring-buffer in RTC memory for persistence in sleep mode
	*rtcLogWritePtr = c;
	if (rtcLogWritePtr >= logBuffEnd) {
		rtcLogWritePtr = rtcLogBuffer;
	} else {
		rtcLogWritePtr++;
	}

	// line buffer for websocket output
	static char line_buff[128] = {'a'};
	static unsigned line_len = 1;
	static char *cur_char = &line_buff[1];

	*cur_char++ = c;
	line_len++;

	if (c == '\n' || line_len >= sizeof(line_buff) - 1) {
		line_buff[sizeof(line_buff) - 1] = '\0';
		if (isLog && wsc && wsc->status() == WS_CONNECTED)
			wsc->binary(line_buff, line_len);
		else
			isLog = false;
		cur_char = &line_buff[1];
		line_len = 1;
	}
}

void wsDumpRtc(AsyncWebSocketClient *ws_client)
{
	if (!ws_client)
		return;

	AsyncWebSocketMessageBuffer *buffer = g_ws.makeBuffer(LOG_FILE_SIZE + 1);
	if (!buffer) {
		log_e("No buffer :(");
		return;
	}
	uint8_t *buf = buffer->get();
	*buf++ = 'a';

	const char *w_ptr = rtcLogWritePtr;

	// dump rtc_buffer[w_ptr:] (oldest part)
	unsigned l0 = logBuffEnd - w_ptr + 1;
	memcpy(buf, w_ptr, l0);
	buf += l0;

	// dump rtc_buffer[:w_ptr] (newest part)
	unsigned l1 = w_ptr - rtcLogBuffer;
	memcpy(buf, rtcLogBuffer, l1);

	log_v("l0 + l1 = %d", l0 + l1);

	ws_client->binary(buffer);

	wsc = ws_client;
	isLog = true;
}

void web_console_init()
{
	if (rtc_get_reset_reason(0) == POWERON_RESET) {
		memset(rtcLogBuffer, 0, LOG_FILE_SIZE - 1);
		rtcLogWritePtr = rtcLogBuffer;
	} else {
		if (rtcLogWritePtr < rtcLogBuffer || rtcLogWritePtr > logBuffEnd)
			rtcLogWritePtr = rtcLogBuffer;
	}
	ets_install_putc2(wsDebugPutc);
}
