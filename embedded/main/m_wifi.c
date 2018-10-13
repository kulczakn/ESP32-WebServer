/*
Copyright (c) 2017 Tony Pottier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file wifi_manager.c
@author Tony Pottier
@brief Defines all functions necessary for esp32 to connect to a wifi/scan wifis

Contains the freeRTOS task and all necessary support

@see https://idyl.io
@see https://github.com/tonyp7/esp32-wifi-manager
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "m_http_server.h"
#include "m_wifi.h"

/**
 * The actual WiFi settings in use
 */
struct wifi_settings_t wifi_settings = {
	.ap_ssid = DEFAULT_AP_SSID,
	.ap_pwd = DEFAULT_AP_PASSWORD,
	.ap_channel = DEFAULT_AP_CHANNEL,
	.ap_ssid_hidden = DEFAULT_AP_SSID_HIDDEN,
	.ap_bandwidth = DEFAULT_AP_BANDWIDTH
};

/* Wifi Task */

TaskHandle_t wifi_task_handle = NULL;

/* WIFI EVENT GROUP */

EventGroupHandle_t wifi_task_event_group;

/* @brief  */
const int WIFI_AP_STA_CONNECTED_BIT = BIT0;

/* @brief Set automatically once the SoftAP is started */
const int WIFI_AP_STARTED = BIT1;

esp_err_t wifi_task_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {

    case SYSTEM_EVENT_AP_START:
    	xEventGroupSetBits(wifi_task_event_group, WIFI_AP_STARTED);
		break;

    case SYSTEM_EVENT_AP_STACONNECTED:
		xEventGroupSetBits(wifi_task_event_group, WIFI_AP_STA_CONNECTED_BIT);
		break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
    	xEventGroupClearBits(wifi_task_event_group, WIFI_AP_STA_CONNECTED_BIT);
		break;

	default:
        break;
    }
	return ESP_OK;
}

uint8_t wifi_ap_init(void)
{
	/* initialize the tcp stack */
	tcpip_adapter_init();

    /* event handler and event group for the wifi driver */
	wifi_task_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_task_event_handler, NULL));

	/* start the softAP access point */
	/* stop DHCP server */
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));

	/* assign a static IP to the AP network interface */
	tcpip_adapter_ip_info_t info;
	memset(&info, 0x00, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));

	/* start dhcp server */
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));

	/* init wifi as station + access point */
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(WIFI_IF_AP, wifi_settings.ap_bandwidth));

	// configure the softAP and start it */
	wifi_config_t ap_config = {
		.ap = {
			.ssid_len = 0,
			.channel = wifi_settings.ap_channel,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.ssid_hidden = wifi_settings.ap_ssid_hidden,
			.max_connection = AP_MAX_CONNECTIONS,
			.beacon_interval = AP_BEACON_INTERVAL,
		},
	};
	memcpy(ap_config.ap.ssid, wifi_settings.ap_ssid , sizeof(wifi_settings.ap_ssid));
	memcpy(ap_config.ap.password, wifi_settings.ap_pwd, sizeof(wifi_settings.ap_pwd));

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	/* wait for access point to start */
	xEventGroupWaitBits(wifi_task_event_group, WIFI_AP_STARTED, pdFALSE, pdTRUE, portMAX_DELAY);

	http_server_set_event_start();

	return 1;
}

void wifi_task( void *pvParameters ){

	while(1)
	{
		vTaskDelay( (TickType_t)500);
	}
}

uint8_t wifi_init(void)
{
	/* start the HTTP Server task */
	xTaskCreate(&wifi_task, "wifi_task", 4096, NULL, 3, &wifi_task_handle);
	wifi_ap_init();

	return 1;
}