#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "m_system.h"
#include "m_module.h"

#define TAG "main.c"

void app_main()
{
	/* This must be first, modules have dependencies on system modules */
	/* Initialize the system modules */
	system_init();

	/* Register modules */

	//

	/* end */

	/* Initialize the module extension framework */
	module_init();
}
