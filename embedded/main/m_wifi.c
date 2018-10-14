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

#include "m_http.h"
#include "m_websocket.h"
#include "m_wifi.h"

/**
 * The actual WiFi settings in use
 */
wifi_settings_t wifi_settings = {
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

/* @brief Indicates that a station has connected to the access point */
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

	return 1;
}


uint8_t wifi_init(void)
{
	uint8_t ret = 0;

	// if the ap succesfully starts the start the websocket and http servers
	if(wifi_ap_init())
	{
		http_server_start();
		websocket_server_start();

		ret = 1;
	}

	return ret;
}

void wifi_wait(void)
{
	xEventGroupWaitBits(wifi_task_event_group, WIFI_AP_STARTED, pdFALSE, pdTRUE, portMAX_DELAY);
}