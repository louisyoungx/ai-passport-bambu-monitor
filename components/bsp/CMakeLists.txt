set(board_sources "src/bsp_button.c" "src/bsp_audio.c" "src/bsp_battery.c")
set(board_requires button esp_adc esp_codec_dev esp_driver_i2s)
if(CONFIG_BSP_BOARD_CYD)
    set(board_sources "src/bsp_input_cyd.c" "src/bsp_audio_cyd.c" "src/bsp_battery_cyd.c")
endif()
# Dependencies are expanded before sdkconfig is available. Use the target,
# rather than CONFIG_*, so both CYD display choices get their include paths.
if(IDF_TARGET STREQUAL "esp32")
    set(board_requires esp_driver_dac esp_lcd_ili9341)
endif()

idf_component_register(
    SRCS "src/bsp_i2c.c" "src/bsp_display.c" "src/bsp_display_lvgl.c"
         "src/bsp_display_rounding.c" ${board_sources}
    INCLUDE_DIRS "include"
    REQUIRES driver esp_driver_i2c esp_lcd esp_lvgl_port ${board_requires}
    PRIV_REQUIRES esp_timer esp_pm
)
