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

#define TAG "m_http.c"

// http server event group
EventGroupHandle_t http_server_event_group;
EventBits_t uxBits;

/* embedded binary data */
extern const uint8_t style_css_start[] asm("_binary_style_css_start");
extern const uint8_t style_css_end[]   asm("_binary_style_css_end");
extern const uint8_t jquery_gz_start[] asm("_binary_jquery_gz_start");
extern const uint8_t jquery_gz_end[] asm("_binary_jquery_gz_end");
extern const uint8_t code_js_start[] asm("_binary_code_js_start");
extern const uint8_t code_js_end[] asm("_binary_code_js_end");
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");


/* const http headers stored in ROM */
const static char http_html_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const static char http_css_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/css\nCache-Control: public, max-age=31536000\n\n";
const static char http_js_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
const static char http_jquery_gz_hdr[] = "HTTP/1.1 200 OK\nContent-type: text/javascript\nAccept-Ranges: bytes\nContent-Length: 29995\nContent-Encoding: gzip\n\n";
const static char http_400_hdr[] = "HTTP/1.1 400 Bad Request\nContent-Length: 0\n\n";
const static char http_404_hdr[] = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";
const static char http_503_hdr[] = "HTTP/1.1 503 Service Unavailable\nContent-Length: 0\n\n";
const static char http_ok_json_no_cache_hdr[] = "HTTP/1.1 200 OK\nContent-type: application/json\nCache-Control: no-store, no-cache, must-revalidate, max-age=0\nPragma: no-cache\n\n";

#define HTTP_ROUTE_HOME = "/"
#define HTTP_ROUTE_CHAT = "/chat"
#define HTTP_ROUTE_ARCADE = "/arcade"


/**
 *	@brief Indicates that the http server should start 
 */
const int HTTP_SERVER_START_BIT = BIT0;

void http_server_start(void)
{
	xEventGroupSetBits(http_server_event_group, HTTP_SERVER_START_BIT);
}

char* http_server_get_header(char *request, char *header_name, int *len) {
	*len = 0;
	char *ret = NULL;
	char *ptr = NULL;

	ptr = strstr(request, header_name);
	if (ptr) {
		ret = ptr + strlen(header_name);
		ptr = ret;
		while (*ptr != '\0' && *ptr != '\n' && *ptr != '\r') {
			(*len)++;
			ptr++;
		}
		return ret;
	}
	return NULL;
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

	http_server_event_group = xEventGroupCreate();

	/* do not start the task until wifi_manager says it's safe to do so! */
	ESP_LOGI(TAG, "waiting for start bit");
	uxBits = xEventGroupWaitBits(http_server_event_group, HTTP_SERVER_START_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
	ESP_LOGI(TAG, "received start bit, starting server\n");

	struct netconn *conn;
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, 80);
	netconn_listen(conn);

	ESP_LOGI(TAG, "HTTP Server listening...");

	do {
		struct netconn *newconn;

		err = netconn_accept(conn, &newconn);

		if (err == ERR_OK) {
			http_handle_new_connection(newconn);
		}

		vTaskDelay( (TickType_t)10); /* allows the freeRTOS scheduler to take over if needed */

	} while(err == ERR_OK);

	// close main connection
	netconn_close(conn);
	netconn_delete(conn);

	ESP_LOGI(TAG, "HTTP server is closing.");
}

void http_server_netconn_serve(struct netconn *conn) {

	ESP_LOGI(TAG, "Handling request from %u.", conn->socket);

	struct netbuf *inbuf;
	char *buf = NULL;
	u16_t buflen;
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
				ESP_LOGI(TAG, "HTTP: Serving '/'");
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, index_html_start, index_html_end - index_html_start, NETCONN_NOCOPY);
			}
			else if(strstr(line, "GET /jquery.js ")) {
				ESP_LOGI(TAG, "HTTP: Serving '/jquery.js'");
				netconn_write(conn, http_jquery_gz_hdr, sizeof(http_jquery_gz_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, jquery_gz_start, jquery_gz_end - jquery_gz_start, NETCONN_NOCOPY);
			}
			else if(strstr(line, "GET /code.js ")) {
				ESP_LOGI(TAG, "HTTP: Serving '/code.js'");
				netconn_write(conn, http_js_hdr, sizeof(http_js_hdr) - 1, NETCONN_NOCOPY);
				netconn_write(conn, code_js_start, code_js_end - code_js_start, NETCONN_NOCOPY);
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
