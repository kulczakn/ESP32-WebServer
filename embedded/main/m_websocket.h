#ifndef	_M_WEBSOCKET_H
#define _M_WEBSOCKET_H

#include "lwip/api.h"

#define WS_MASK_L		0x4		/**< \brief Length of MASK field in WebSocket Header*/


/** \brief Websocket frame header type*/
typedef struct {
	uint8_t opcode :WS_MASK_L;
	uint8_t reserved :3;
	uint8_t FIN :1;
	uint8_t payload_length :7;
	uint8_t mask :1;
} WS_frame_header_t;

/** \brief Websocket frame type*/
typedef struct{
	struct netconn* 	conenction;
	WS_frame_header_t	frame_header;
	size_t				payload_length;
	char*				payload;
} WebSocket_frame_t;


/**
 * \brief Send data to the websocket client
 *
 * \return 	#ERR_VAL: 	Payload length exceeded 2^7 bytes.
 * 			#ERR_CONN:	There is no open connection
 * 			#ERR_OK:	Header and payload send
 * 			all other values: derived from #netconn_write (sending frame header or payload)
 */
err_t websocket_write_data(char* p_data, size_t length);

/**
 * \brief WebSocket Server task
 */
void websocket_server_task(void *pvParameters);

/**
 *	\brief WebSocket processing task
 */
void websocket_process_task(void *pvParameters);


#endif /* _WEBSOCKET_TASK_H_ */
