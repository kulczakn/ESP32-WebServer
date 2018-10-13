deps_config := \
	/home/nate/esp/esp-idf/components/app_trace/Kconfig \
	/home/nate/esp/esp-idf/components/aws_iot/Kconfig \
	/home/nate/esp/esp-idf/components/bt/Kconfig \
	/home/nate/esp/esp-idf/components/driver/Kconfig \
	/home/nate/esp/esp-idf/components/esp32/Kconfig \
	/home/nate/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/nate/esp/esp-idf/components/esp_http_client/Kconfig \
	/home/nate/esp/esp-idf/components/ethernet/Kconfig \
	/home/nate/esp/esp-idf/components/fatfs/Kconfig \
	/home/nate/esp/esp-idf/components/freertos/Kconfig \
	/home/nate/esp/esp-idf/components/heap/Kconfig \
	/home/nate/esp/esp-idf/components/http_server/Kconfig \
	/home/nate/esp/esp-idf/components/libsodium/Kconfig \
	/home/nate/esp/esp-idf/components/log/Kconfig \
	/home/nate/esp/esp-idf/components/lwip/Kconfig \
	/home/nate/esp/esp-idf/components/mbedtls/Kconfig \
	/home/nate/esp/esp-idf/components/mdns/Kconfig \
	/home/nate/esp/esp-idf/components/mqtt/Kconfig \
	/home/nate/esp/esp-idf/components/nvs_flash/Kconfig \
	/home/nate/esp/esp-idf/components/openssl/Kconfig \
	/home/nate/esp/esp-idf/components/pthread/Kconfig \
	/home/nate/esp/esp-idf/components/spi_flash/Kconfig \
	/home/nate/esp/esp-idf/components/spiffs/Kconfig \
	/home/nate/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/nate/esp/esp-idf/components/vfs/Kconfig \
	/home/nate/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/nate/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/nate/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/nate/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/nate/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
