#ifndef _M_SYSTEM_H
#define _M_SYSTEM_H

#define M_SYSTEM_DEBUG 1

#define M_SYSTEM_MAX_ROUTE_LENGTH 10
#define M_SYSTEM_MAX_SUBROUTE_LENGTH 10
#define M_SYSTEM_MAX_DATA_LENGTH 32

typedef uint8_t client_t;

/* System message type used to pass messages between modules */
typedef struct {
	const client_t client_id;
	const char route[M_SYSTEM_MAX_ROUTE_LENGTH];
	const char subroute[M_SYSTEM_MAX_SUBROUTE_LENGTH];
	const char data[M_SYSTEM_MAX_DATA_LENGTH];
} m_system_message_t;

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