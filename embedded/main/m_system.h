#ifndef _M_SYSTEM_H
#define _M_SYSTEM_H

#define M_SYSTEM_DEBUG 1

/**
 *	@brief initialize system modules:
 *	HTTP Server
 *	Websocket Server
 */
void system_init(void);

void system_wifi_wait(void);
void system_http_wait(void);
void system_websocket_wait(void);

#endif