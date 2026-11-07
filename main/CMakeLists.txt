set(console_requires)
if(IDF_TARGET STREQUAL "esp32c3")
    list(APPEND console_requires esp_driver_usb_serial_jtag)
endif()

idf_component_register(
    SRCS "main.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "app_wifi.c"
         "app_wifi_menu.c"
         "app_mqtt.c"
         "app_notify.c"
         "app_boot.c"
         "app_printer.c"
         "app_config.c"
         "app_provision.c"
         "hms_codes.c"
         "logo_bambu.c"
         "font_zh14.c"
         "font_zh20.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_wifi nvs_flash esp_event esp_netif esp_timer esp_pm ${console_requires} esp_http_server esp_http_client mqtt json mbedtls
)

# Firmware version from ESP-IDF's project metadata, for the device page.
idf_build_get_property(firmware_version PROJECT_VER)
target_compile_definitions(${COMPONENT_LIB} PUBLIC FW_VERSION="${firmware_version}")
