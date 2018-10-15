#include "string.h"

#include "nvs_flash.h"
#include "esp_log.h"

#include "m_wifi.h"
#include "m_websocket.h"
#include "m_http.h"
#include "m_system.h"
#include "m_module.h"

#define TAG "m_system.c"

/* Array of created module queues, index by their module_t ID assigned at their registration */
QueueHandle_t module_queues[MODULE_MAX_COUNT];

/* Array of module routes, index by their module_t ID and used to map routes to their message queue */ 
/* TODO: Make this a hash table of some sort this is just lazy */
char module_routes[MODULE_MAX_COUNT][M_SYSTEM_MAX_ROUTE_LENGTH];

/**
 *	@brief Monitoring task that prints out free heap
 */
void system_monitoring_task(void *pvParameter)
{
	while(1)
	{
		printf("free heap: %d\n", esp_get_free_heap_size());
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

/**
 *	@brief initialize system modules:
 *	NVS Flash
 *	Wifi Access Point
 *	HTTP Server
 *	Websocket Server
 */
void system_init(void)
{
	/* Starts memory debug task */
	if(M_SYSTEM_DEBUG)
	{
		xTaskCreatePinnedToCore(&system_monitoring_task, "system_monitoring_task", 2048, NULL, 1, NULL, 1);
		ESP_LOGI(TAG, "Monitoring task started.");
	}

	/* initialize flash memory */
	nvs_flash_init();

	/* Initialize the http module */
	http_init();
	ESP_LOGI(TAG, "Http server module initialized.");

	/* Initialize the websocket module */
	websocket_init();
	ESP_LOGI(TAG, "Websocket module initialized");

	/* Initialize wifi access point */
	wifi_init();
	ESP_LOGI(TAG, "Wifi access point module initialized.");

	/* Initialize the system queue features */

}

/* Wait for HTTP server to start */
void system_http_wait(void)
{
	http_wait();
}

/* Wait for websocket server to start */
void system_websocket_wait(void)
{
	websocket_wait();
}

/* Wait for wifi AP to start */
void system_wifi_wait(void)
{
	wifi_wait();
}

/**
 *	@brief Called by the websocket server to pass messages to system module
 */
void system_handle_message(const char *raw_message)
{
	// parse the message with a json parser and switch off the route
	return;
}

/**
 *	@brief Creates and returns the handler to a new message queue that modules can use to recieve messages from the websocket server
 */
QueueHandle_t system_message_queue(m_module_t module, const char *route)
{
	module_queues[module] = xQueueCreate(10, sizeof(m_system_message_t));
	strcpy(module_routes[module], route);
}