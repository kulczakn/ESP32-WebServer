#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/opt.h"
#include "lwip/memp.h"
#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/priv/api_msg.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/priv/tcpip_priv.h"

#include "m_http.h"
#include "m_wifi.h"
#include "m_system.h"

#define TAG "m_http.c"

// http server event group
TaskHandle_t task_http_server = NULL;
EventGroupHandle_t http_server_event_group;
EventBits_t uxBits;

/* embedded binary data */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");
extern const uint8_t chat_html_start[] asm("_binary_chat_html_start");
extern const uint8_t chat_html_end[] asm("_binary_chat_html_end");
extern const uint8_t game_html_start[] asm("_binary_game_html_start");
extern const uint8_t game_html_end[] asm("_binary_game_html_end");


/* const http headers stored in ROM */
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_400_hdr[] = "HTTP/1.1 400 Bad Request\nContent-Length: 0\n\n";
const static char http_404_hdr[] = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";

#define HTTP_ROUTE_INDEX = "/"
#define HTTP_ROUTE_CHAT = "/chat"
#define HTTP_ROUTE_GAME = "/game"

/**
 *	@brief Indicates that the HTTP server has started
 */
const int HTTP_SERVER_STARTED = BIT0;

void http_server_netconn_serve(struct netconn *conn) {

	ESP_LOGI(TAG, "Handling request from %u.", conn->socket);

	struct netbuf *inbuf;
	char *buf = NULL;
	uint16_t buflen;
	err_t err;
	const char new_line[2] = "\n";

	err = netconn_recv(conn, &inbuf);
	if (err == ERR_OK) {

		netbuf_data(inbuf, (void**)&buf, &buflen);

		/* extract the first line of the request */
		char *save_ptr = buf;
		char *line = strtok_r(save_ptr, new_line, &save_ptr);

		if(line) {

			// default page
			if(strstr(line, "GET / ")) {
				ESP_LOGI(TAG, "HTTP: Serving 'index.html'");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, index_html_start, index_html_end - index_html_start, NETCONN_NOCOPY);
			}
			else if(strstr(line, "GET /chat ")) {
				ESP_LOGI(TAG, "HTTP: Serving 'chat.html'");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, chat_html_start, chat_html_end - chat_html_start, NETCONN_NOCOPY);
			}
			else if(strstr(line, "GET /game ")) {
				ESP_LOGI(TAG, "HTTP: Serving 'game.html'");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, game_html_start, game_html_end - game_html_start, NETCONN_NOCOPY);
			}
			else{
				netconn_write(conn, http_400_hdr, sizeof(http_400_hdr) - 1, NETCONN_NOCOPY);
			}
		}
		else{
			netconn_write(conn, http_404_hdr, sizeof(http_404_hdr) - 1, NETCONN_NOCOPY);
		}
	}

	netbuf_delete(inbuf);

	ESP_LOGI(TAG, "Request handled from %u.", conn->socket);
}

void http_connection_handle_task(void *pvParameters)
{
	struct netconn *conn = (struct netconn*)pvParameters;

	http_server_netconn_serve(conn);

	ESP_LOGI(TAG, "Page served to %u, deleting connection.", conn->socket);
	netconn_delete(conn);

	ESP_LOGI(TAG, "Deleting HTTP handler task.");
	vTaskDelete(NULL);
}

void http_handle_new_connection(struct netconn* conn)
{
	// create a task to handle the new websocket connection
    xTaskCreate(&http_connection_handle_task, "http_process", 2048, (void*)conn, 5, NULL);
    ESP_LOGI(TAG, "HTTP handler for %u started.", conn->socket);
}


void http_server_task(void *pvParameters) {
	/* do not start the task until wifi_manager says it's safe to do so! */
	ESP_LOGI(TAG, "waiting for wifi ap to start.");
	system_wifi_wait();

	struct netconn *conn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, 80);
	netconn_listen(conn);

	xEventGroupSetBits(http_server_event_group, HTTP_SERVER_STARTED);
	ESP_LOGI(TAG, "HTTP Server listening...");

	do {
		struct netconn *newconn;

		err = netconn_accept(conn, &newconn);

		if (err == ERR_OK) {
			http_handle_new_connection(newconn);
		}

		vTaskDelay( (TickType_t)10);

	} while(err == ERR_OK);

	// close main connection
	netconn_close(conn);
	netconn_delete(conn);

	ESP_LOGI(TAG, "HTTP server is closing.");
}

void http_init(void)
{
	/* Create event group for waiting and stuff */
	http_server_event_group = xEventGroupCreate();

	/* start the HTTP Server task */
	xTaskCreate(&http_server_task, "http_server_task", 2048, NULL, 5, &task_http_server);
}

/**
 *	@brief block and wait for the http server to start
 */
void http_wait(void)
{
	xEventGroupWaitBits(http_server_event_group, HTTP_SERVER_STARTED, pdFALSE, pdTRUE, portMAX_DELAY);
}