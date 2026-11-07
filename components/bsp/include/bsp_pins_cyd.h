// Common ESP32-2432S028R wiring. No PSRAM, ES8311 or CW2017.
#pragma once

#define BSP_LCD_W            240
#define BSP_LCD_H            320
#define BSP_LCD_SPI_HOST     SPI2_HOST
#define BSP_LCD_MOSI         13
#define BSP_LCD_SCLK         14
#define BSP_LCD_CS           15
#define BSP_LCD_DC           2
#define BSP_LCD_RST          (-1)
#define BSP_LCD_BL           21
#define BSP_LCD_PCLK_HZ      (40 * 1000 * 1000)
#define BSP_LCD_SPI_MODE     CONFIG_BSP_CYD_LCD_SPI_MODE
#if CONFIG_BSP_CYD_LCD_INVERT
#define BSP_LCD_INVERT_COLOR 1
#else
#define BSP_LCD_INVERT_COLOR 0
#endif
#if CONFIG_BSP_CYD_LCD_BGR
#define BSP_LCD_RGB_ORDER    LCD_RGB_ELEMENT_ORDER_BGR
#else
#define BSP_LCD_RGB_ORDER    LCD_RGB_ELEMENT_ORDER_RGB
#endif
#if CONFIG_BSP_CYD_LCD_MIRROR_X
#define BSP_LCD_NATIVE_MIRROR_X 1
#else
#define BSP_LCD_NATIVE_MIRROR_X 0
#endif
#if CONFIG_BSP_CYD_LCD_MIRROR_Y
#define BSP_LCD_NATIVE_MIRROR_Y 1
#else
#define BSP_LCD_NATIVE_MIRROR_Y 0
#endif
#if CONFIG_BSP_CYD_ROTATE_180
#define BSP_LCD_ROTATE_180   1
#else
#define BSP_LCD_ROTATE_180   0
#endif
// A half turn toggles both axes after the panel's native mirror correction.
#define BSP_LCD_MIRROR_X (BSP_LCD_NATIVE_MIRROR_X ^ BSP_LCD_ROTATE_180)
#define BSP_LCD_MIRROR_Y (BSP_LCD_NATIVE_MIRROR_Y ^ BSP_LCD_ROTATE_180)

#define BSP_TOUCH_SPI_HOST   SPI3_HOST
#define BSP_TOUCH_SCLK       25
#define BSP_TOUCH_MOSI       32
#define BSP_TOUCH_MISO       39
#define BSP_TOUCH_CS         33
#define BSP_TOUCH_IRQ        36  // Input only, no internal pull-up.
#define BSP_BTN_GPIO         0
#define BSP_BTN_COUNT        3  // Logical UP/DOWN/OK, supplied by gestures.
#define BSP_BTN_SHORT_PRESS_MS 180
#define BSP_BTN_LONG_PRESS_MS  500
#define BSP_BL_LEDC_TIMER    LEDC_TIMER_0
#define BSP_BL_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BSP_BL_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BSP_BL_LEDC_RES      LEDC_TIMER_10_BIT
#define BSP_BL_LEDC_FREQ_HZ  5000
#define BSP_I2C_PORT         I2C_NUM_0
#define BSP_I2C_SDA          27
#define BSP_I2C_SCL          22
#define BSP_I2C_ES8311_ADDR  0x18
#define BSP_I2C_CW2017_ADDR  0x63
#define BSP_AUDIO_GPIO       26
#define BSP_LED_RED          4
#define BSP_LED_GREEN        16
#define BSP_LED_BLUE         17
