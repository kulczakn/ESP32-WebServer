#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "m_http_server.h"
#include "m_wifi.h"
#include "m_websocket.h"

#define TAG "main.c"

TaskHandle_t task_http_server = NULL;

/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code!
 */
void monitoring_task(void *pvParameter)
{
	while(1)
	{
		printf("free heap: %d\n",esp_get_free_heap_size());
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

void app_main()
{
	// /* disable the default wifi logging */
	// esp_log_level_set("wifi", ESP_LOG_NONE);

	/* initialize flash memory */
	nvs_flash_init();

	/* start the HTTP Server task */
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &task_http_server);
	ESP_LOGI(TAG, "Http server task started.");

	/* Initialize wifi access point */
	wifi_init();
	ESP_LOGI(TAG, "Wifi access point task started.");

	//create WebSocker receive task
    xTaskCreate(&websocket_process_task, "ws_process_rx", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Websocket processing task started.");

    //Create Websocket Server Task
    xTaskCreate(&websocket_server_task, "ws_server", 2048, NULL, 5, NULL);
    ESP_LOGI(TAG, "Websocket server task started.");

	/* your code should go here. In debug mode we create a simple task on core 2 that monitors free heap memory */
#if WIFI_MANAGER_DEBUG
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
	ESP_LOGI(TAG, "Monitoring task started.");
#endif


}
