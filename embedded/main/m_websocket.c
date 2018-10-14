#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_heap_caps.h"
#include "hwcrypto/sha.h"
#include "esp_system.h"
#include "esp_log.h"
#include "wpa2/utils/base64.h"

#include "m_websocket.h"
#include "m_chat.h"

#define TAG "m_websocket.c"

#define WS_PORT				9998	/**< \brief TCP Port for the Server*/
#define WS_CLIENT_KEY_L		24		/**< \brief Length of the Client Key*/
#define SHA1_RES_L			20		/**< \brief SHA1 result*/
#define WS_STD_LEN			125		/**< \brief Maximum Length of standard length frames*/
#define WS_SPRINTF_ARG_L	4		/**< \brief Length of sprintf argument for string (%.*s)*/

/** 
 *	\brief Opcode according to RFC 6455
 */
typedef enum {
	WS_OP_CON = 0x0, 				/*!< Continuation Frame*/
	WS_OP_TXT = 0x1, 				/*!< Text Frame*/
	WS_OP_BIN = 0x2, 				/*!< Binary Frame*/
	WS_OP_CLS = 0x8, 				/*!< Connection Close Frame*/
	WS_OP_PIN = 0x9, 				/*!< Ping Frame*/
	WS_OP_PON = 0xa 				/*!< Pong Frame*/
} WS_OPCODES;

// Reference to the RX queue
QueueHandle_t websocket_rx_queue;

// Web socket event group
EventGroupHandle_t websocket_server_event_group;
static EventBits_t uxBits;

// Reference to open websocket connection
static struct netconn* WS_conn = NULL;

const char WS_sec_WS_keys[] = "Sec-WebSocket-Key:";
const char WS_sec_conKey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char WS_srv_hs[] ="HTTP/1.1 101 Switching Protocols \r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %.*s\r\n\r\n";

/**
 *	@brief Indicates that the http server should start 
 */
const int WEBSOCKET_SERVER_START_BIT = BIT0;

void websocket_server_start(void)
{
	xEventGroupSetBits(websocket_server_event_group, WEBSOCKET_SERVER_START_BIT);
}

/**
 *	@brief Writes the data to the websocket connection
 */
err_t websocket_write_data(struct netconn* conn, char* data, size_t length) 
{

	//check if we have an open connection
	if (conn == NULL)
		return ERR_CONN;

	//currently only frames with a payload length <WS_STD_LEN are supported
	if (length > WS_STD_LEN)
		return ERR_VAL;

	//netconn_write result buffer
	err_t result;

	//prepare header
	WS_frame_header_t hdr;
	hdr.FIN = 0x1;
	hdr.payload_length = length;
	hdr.mask = 0;
	hdr.reserved = 0;
	hdr.opcode = WS_OP_TXT;

	//send header
	result = netconn_write(conn, &hdr, sizeof(WS_frame_header_t), NETCONN_COPY);

	//check if header was send
	if (result != ERR_OK)
		return result;

	//send payload
	return netconn_write(conn, data, length, NETCONN_COPY);
}

/**
 *	@brief this task reads the websocket frame queue and handles each frame
 */
void websocket_process_task(void *pvParameters)
{
    //frame buffer
    WebSocket_frame_t rx_frame;

    while(1)
    {
        //receive next WebSocket frame from queue
        if(xQueueReceive(websocket_rx_queue, &rx_frame, 3*portTICK_PERIOD_MS)==pdTRUE){

        	//write frame information to serial out
        	ESP_LOGI(TAG, "WS Frame: %.*s", rx_frame.payload_length, rx_frame.payload);

        	//loop back frame
        	ESP_LOGI(TAG, "Looping back frame.");
        	// websocket_write_data(rx_frame.connection, rx_frame.payload, rx_frame.payload_length);

        	chat_handle_message(rx_frame);

        	//free memory
			if (rx_frame.payload != NULL)
			{
				ESP_LOGI(TAG, "Freeing frame.");
				free(rx_frame.payload);
			}
        }
    }
}

static void websocket_server_netconn_serve(struct netconn *conn) 
{

	//Netbuf
	struct netbuf *inbuf;

	//message buffer
	char *buf;

	//pointer to buffer (multi purpose)
	char* p_buf;

	//Pointer to SHA1 input
	char* p_SHA1_Inp;

	//Pointer to SHA1 result
	char* p_SHA1_result;

	//multi purpose number buffer
	uint16_t i;

	//will point to payload (send and receive
	char* p_payload;

	//Frame header pointer
	WS_frame_header_t* p_frame_hdr;

	//allocate memory for SHA1 input
	p_SHA1_Inp = heap_caps_malloc(WS_CLIENT_KEY_L + sizeof(WS_sec_conKey),
			MALLOC_CAP_8BIT);

	//allocate memory for SHA1 result
	p_SHA1_result = heap_caps_malloc(SHA1_RES_L, MALLOC_CAP_8BIT);

	//Check if malloc suceeded
	if ((p_SHA1_Inp != NULL) && (p_SHA1_result != NULL)) {

		//receive handshake request
		if (netconn_recv(conn, &inbuf) == ERR_OK) {

			//read buffer
			netbuf_data(inbuf, (void**) &buf, &i);

			//write static key into SHA1 Input
			for (i = 0; i < sizeof(WS_sec_conKey); i++)
				p_SHA1_Inp[i + WS_CLIENT_KEY_L] = WS_sec_conKey[i];

			//find Client Sec-WebSocket-Key:
			p_buf = strstr(buf, WS_sec_WS_keys);

			//check if needle "Sec-WebSocket-Key:" was found
			if (p_buf != NULL) {

				//get Client Key
				for (i = 0; i < WS_CLIENT_KEY_L; i++)
					p_SHA1_Inp[i] = *(p_buf + sizeof(WS_sec_WS_keys) + i);

				// calculate hash
				esp_sha(SHA1, (unsigned char*) p_SHA1_Inp, strlen(p_SHA1_Inp),
						(unsigned char*) p_SHA1_result);

				//hex to base64
				p_buf = (char*) base64_encode((unsigned char*) p_SHA1_result,
						SHA1_RES_L, (size_t*) &i);

				//free SHA1 input
				free(p_SHA1_Inp);

				//free SHA1 result
				free(p_SHA1_result);

				//allocate memory for handshake
				p_payload = heap_caps_malloc(
						sizeof(WS_srv_hs) + i - WS_SPRINTF_ARG_L,
						MALLOC_CAP_8BIT);

				//check if malloc suceeded
				if (p_payload != NULL) {

					//prepare handshake
					sprintf(p_payload, WS_srv_hs, i - 1, p_buf);

					//send handshake
					netconn_write(conn, p_payload, strlen(p_payload),
							NETCONN_COPY);

					//free base 64 encoded sec key
					free(p_buf);

					//free handshake memory
					free(p_payload);

					//set pointer to open WebSocket connection
					WS_conn = conn;

					//Wait for new data
					while (netconn_recv(conn, &inbuf) == ERR_OK) {

						//read data from inbuf
						netbuf_data(inbuf, (void**) &buf, &i);

						//get pointer to header
						p_frame_hdr = (WS_frame_header_t*) buf;

						//check if clients wants to close the connection
						if (p_frame_hdr->opcode == WS_OP_CLS)
							break;

						//get payload length
						if (p_frame_hdr->payload_length <= WS_STD_LEN) {

							//get beginning of mask or payload
							p_buf = (char*) &buf[sizeof(WS_frame_header_t)];

							//check if content is masked
							if (p_frame_hdr->mask) {

								//allocate memory for decoded message
								p_payload = heap_caps_malloc(
										p_frame_hdr->payload_length + 1,
										MALLOC_CAP_8BIT);

								//check if malloc succeeded
								if (p_payload != NULL) {

									//decode playload
									for (i = 0; i < p_frame_hdr->payload_length;
											i++)
										p_payload[i] = (p_buf + WS_MASK_L)[i]
												^ p_buf[i % WS_MASK_L];
												
									//add 0 terminator
									p_payload[p_frame_hdr->payload_length] = 0;
								}
							} else
								//content is not masked
								p_payload = p_buf;

							//do stuff
							if ((p_payload != NULL)	&& (p_frame_hdr->opcode == WS_OP_TXT)) {

								//prepare FreeRTOS message
								WebSocket_frame_t ws_frame;
								ws_frame.connection=conn;
								ws_frame.frame_header=*p_frame_hdr;
								ws_frame.payload_length=p_frame_hdr->payload_length;
								ws_frame.payload=p_payload;

								//send message
								xQueueSendFromISR(websocket_rx_queue, &ws_frame, 0);
							}

							// free payload buffer (in this demo done by the receive task)
							// if (p_frame_hdr->mask && p_payload != NULL)
							// free(p_payload);

						} //p_frame_hdr->payload_length<126

						//free input buffer
						netbuf_delete(inbuf);

					} //while(netconn_recv(conn, &inbuf)==ERR_OK)
				} //p_payload!=NULL
			} //check if needle "Sec-WebSocket-Key:" was found
		} //receive handshake
	} //p_SHA1_Inp!=NULL&p_SHA1_result!=NULL

	//release pointer to open WebSocket connection
	WS_conn = NULL;

	//delete buffer
	netbuf_delete(inbuf);
}

void websocket_connection_handle_task(void *pvParameters)
{
	struct netconn *conn = (struct netconn*)pvParameters;

	int chat_id = chat_connection_add(conn);

	websocket_server_netconn_serve(conn);

	if(chat_connection_remove(chat_id) != -1)
	{
		ESP_LOGI(TAG, "Removed %u from chatroom", chat_id);
	}

	ESP_LOGI(TAG, "Websocket ending with %u.", conn->socket);
	netconn_close(conn);
	netconn_delete(conn);

	ESP_LOGI(TAG, "Deleting websocket handler task.");
	vTaskDelete(NULL);
}

void websocket_handle_new_connection(struct netconn* conn)
{
	// create a task to handle the new websocket connection
    xTaskCreate(&websocket_connection_handle_task, "ws_process_rx", 2048, (void*)conn, 5, NULL);
    ESP_LOGI(TAG, "Websocket handler for %u started.", conn->socket);
}

void websocket_server_task(void *pvParameters)
{

	// Do not start the websocket server until it has been indicated to do so
	ESP_LOGI(TAG, "waiting for start bit\n");
	uxBits = xEventGroupWaitBits(websocket_server_event_group, WEBSOCKET_SERVER_START_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "received start bit, starting server\n");

	//connection references
	struct netconn *conn;
	err_t err;

	//set up new TCP listener
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, WS_PORT);
	netconn_listen(conn);

	ESP_LOGI(TAG, "Websocket Server listening...");

	do {
		struct netconn *newconn;

		err = netconn_accept(conn, &newconn);

		if (err == ERR_OK) {
			websocket_handle_new_connection(newconn);
		}

		vTaskDelay( (TickType_t)10); /* allows the freeRTOS scheduler to take over if needed */

	} while(err == ERR_OK);

	// close main connection
	netconn_close(conn);
	netconn_delete(conn);

	ESP_LOGI(TAG, "Websocket server is closing.");
}


uint8_t websocket_init(void)
{
	// create the websocket event group
	websocket_server_event_group = xEventGroupCreate();
	ESP_LOGI(TAG, "Websocket event group created.");

	// create the websocket rx queue
    websocket_rx_queue = xQueueCreate(10, sizeof(WebSocket_frame_t));
    ESP_LOGI(TAG, "Websocket rx queue created.");

	// create websocket receive task
    xTaskCreate(&websocket_process_task, "ws_process_rx", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Websocket processing task started.");

    // create websocket server task
    xTaskCreate(&websocket_server_task, "ws_server", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Websocket server task started.");

    return 1;
}