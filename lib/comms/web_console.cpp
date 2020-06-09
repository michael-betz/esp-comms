#include <stdio.h>
#include <stdarg.h>
#include "esp_attr.h"
#include "rom/rtc.h"
#include "comms.h"
#include "web_console.h"

// put the buffer and write pointer in RTC memory,
// such that it survives sleep mode
RTC_NOINIT_ATTR static char rtcLogBuffer[LOG_FILE_SIZE];
RTC_NOINIT_ATTR static char *rtcLogWritePtr = rtcLogBuffer;

static bool isLog = false;

void wsDisableLog()
{
	isLog = false;
}

void wsDebugPutc(char c)
{
	// ring-buffer in RTC memory for persistence in sleep mode
	static const char *logBuffEnd = rtcLogBuffer + LOG_FILE_SIZE - 1;
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
		if (isLog) {
			if (g_ws_client && g_ws_client->available())
				g_ws_client->send(line_buff, line_len);
			else
				wsDisableLog();
		}
		cur_char = &line_buff[1];
		line_len = 1;
	}
}

void wsDumpRtc(void)
{
	const char *logBuffEnd = rtcLogBuffer + LOG_FILE_SIZE - 1;
	if (g_ws_client == NULL)
		return;
	// put ws command on oldest character
	*rtcLogWritePtr = 'a';
	g_ws_client->stream();
	// dump rtc_buffer[w_ptr:] (oldest part)
	g_ws_client->send(rtcLogWritePtr, logBuffEnd - rtcLogWritePtr + 1);
	// dump rtc_buffer[:w_ptr] (newest part)
	g_ws_client->send(rtcLogBuffer, rtcLogWritePtr - rtcLogBuffer);
	g_ws_client->end();
	isLog = true;
}

void web_console_init()
{
	if (rtc_get_reset_reason(0) == POWERON_RESET) {
		memset(rtcLogBuffer, 0, LOG_FILE_SIZE - 1);
		rtcLogWritePtr = rtcLogBuffer;
	} else {
		if (rtcLogWritePtr < rtcLogBuffer || rtcLogWritePtr > &rtcLogBuffer[LOG_FILE_SIZE - 1])
			rtcLogWritePtr = rtcLogBuffer;
	}
	ets_install_putc2(wsDebugPutc);
}
