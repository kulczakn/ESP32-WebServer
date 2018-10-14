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

#include "m_http.h"
#include "m_wifi.h"
#include "m_websocket.h"

#define TAG "main.c"

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

	/* Initialize the module extension framework */
	module_init();

	/* Initialize the http module */
	http_init();
	// ESP_LOGI(TAG, "Http server task started.");

	/* Initialize the websocket module */
	websocket_init();
	ESP_LOGI(TAG, "Websocket module initialized");

	/* Initialize wifi access point */
	wifi_init();
	ESP_LOGI(TAG, "Wifi access point task started.");

	// debug monitoring task
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
	ESP_LOGI(TAG, "Monitoring task started.");

}
