// const char wifi_manager_nvs_namespace[] = "espwifimgr";

// esp_err_t wifi_manager_save_sta_config(){

// 	nvs_handle handle;
// 	esp_err_t esp_err;
// #if WIFI_MANAGER_DEBUG
// 	printf("wifi_manager: About to save config to flash\n");
// #endif
// 	if(wifi_manager_config_sta){

// 		esp_err = nvs_open(wifi_manager_nvs_namespace, NVS_READWRITE, &handle);
// 		if (esp_err != ESP_OK) return esp_err;

// 		esp_err = nvs_set_blob(handle, "ssid", wifi_manager_config_sta->sta.ssid, 32);
// 		if (esp_err != ESP_OK) return esp_err;

// 		esp_err = nvs_set_blob(handle, "password", wifi_manager_config_sta->sta.password, 64);
// 		if (esp_err != ESP_OK) return esp_err;

// 		esp_err = nvs_set_blob(handle, "settings", &wifi_settings, sizeof(wifi_settings));
// 		if (esp_err != ESP_OK) return esp_err;

// 		esp_err = nvs_commit(handle);
// 		if (esp_err != ESP_OK) return esp_err;

// 		nvs_close(handle);
// #if WIFI_MANAGER_DEBUG
// 		printf("wifi_manager_wrote wifi_sta_config: ssid:%s password:%s\n",wifi_manager_config_sta->sta.ssid,wifi_manager_config_sta->sta.password);
// 		printf("wifi_manager_wrote wifi_settings: SoftAP_ssid: %s\n",wifi_settings.ap_ssid);
// 		printf("wifi_manager_wrote wifi_settings: SoftAP_pwd: %s\n",wifi_settings.ap_pwd);
// 		printf("wifi_manager_wrote wifi_settings: SoftAP_channel: %i\n",wifi_settings.ap_channel);
// 		printf("wifi_manager_wrote wifi_settings: SoftAP_hidden (1 = yes): %i\n",wifi_settings.ap_ssid_hidden);
// 		printf("wifi_manager_wrote wifi_settings: SoftAP_bandwidth (1 = 20MHz, 2 = 40MHz): %i\n",wifi_settings.ap_bandwidth);
// 		printf("wifi_manager_wrote wifi_settings: sta_only (0 = APSTA, 1 = STA when connected): %i\n",wifi_settings.sta_only);
// 		printf("wifi_manager_wrote wifi_settings: sta_power_save (1 = yes): %i\n",wifi_settings.sta_power_save);
// 		printf("wifi_manager_wrote wifi_settings: sta_static_ip (0 = dhcp client, 1 = static ip): %i\n",wifi_settings.sta_static_ip);
// 		printf("wifi_manager_wrote wifi_settings: sta_ip_addr: %s\n", ip4addr_ntoa(&wifi_settings.sta_static_ip_config.ip));
// 		printf("wifi_manager_wrote wifi_settings: sta_gw_addr: %s\n", ip4addr_ntoa(&wifi_settings.sta_static_ip_config.gw));
// 		printf("wifi_manager_wrote wifi_settings: sta_netmask: %s\n", ip4addr_ntoa(&wifi_settings.sta_static_ip_config.netmask));
// #endif
// 	}

// 	return ESP_OK;
// }

// bool wifi_manager_fetch_wifi_sta_config(){

// 	nvs_handle handle;
// 	esp_err_t esp_err;
// 	if(nvs_open(wifi_manager_nvs_namespace, NVS_READONLY, &handle) == ESP_OK){

// 		if(wifi_manager_config_sta == NULL){
// 			wifi_manager_config_sta = (wifi_config_t*)malloc(sizeof(wifi_config_t));
// 		}
// 		memset(wifi_manager_config_sta, 0x00, sizeof(wifi_config_t));
// 		memset(&wifi_settings, 0x00, sizeof(struct wifi_settings_t));

// 		/* allocate buffer */
// 		size_t sz = sizeof(wifi_settings);
// 		uint8_t *buff = (uint8_t*)malloc(sizeof(uint8_t) * sz);
// 		memset(buff, 0x00, sizeof(sz));

// 		/* ssid */
// 		sz = sizeof(wifi_manager_config_sta->sta.ssid);
// 		esp_err = nvs_get_blob(handle, "ssid", buff, &sz);
// 		if(esp_err != ESP_OK){
// 			free(buff);
// 			return false;
// 		}
// 		memcpy(wifi_manager_config_sta->sta.ssid, buff, sz);

// 		/* password */
// 		sz = sizeof(wifi_manager_config_sta->sta.password);
// 		esp_err = nvs_get_blob(handle, "password", buff, &sz);
// 		if(esp_err != ESP_OK){
// 			free(buff);
// 			return false;
// 		}
// 		memcpy(wifi_manager_config_sta->sta.password, buff, sz);

// 		/* settings */
// 		sz = sizeof(wifi_settings);
// 		esp_err = nvs_get_blob(handle, "settings", buff, &sz);
// 		if(esp_err != ESP_OK){
// 			free(buff);
// 			return false;
// 		}
// 		memcpy(&wifi_settings, buff, sz);

// 		free(buff);
// 		nvs_close(handle);

// 		return wifi_manager_config_sta->sta.ssid[0] != '\0';


// 	}
// 	else{
// 		return false;
// 	}

// }