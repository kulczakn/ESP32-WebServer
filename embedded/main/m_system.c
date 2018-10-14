#include "nvs_flash.h"
#include "esp_log.h"

#include "m_wifi.h"
#include "m_websocket.h"
#include "m_http.h"
#include "m_system.h"

#define TAG "m_system.c"

/**
 *	@brief Monitoring task that prints out free heap
 */
void system_monitoring_task(void *pvParameter)
{
	while(1)
	{
		printf("free heap: %d\n",esp_get_free_heap_size());
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

	if(M_SYSTEM_DEBUG)
	{
		xTaskCreatePinnedToCore(&system_monitoring_task, "system_monitoring_task", 2048, NULL, 1, NULL, 1);
		ESP_LOGI(TAG, "Monitoring task started.");
	}
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