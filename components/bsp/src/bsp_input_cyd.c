// XPT2046 gestures and BOOT translate to the existing UP/DOWN/OK interface.
// No UI work runs here: the application callback only queues events.
#include "bsp_button.h"
#include "bsp_pins.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>

static const char *TAG = "bsp_input";
static spi_device_handle_t s_touch;
static TaskHandle_t s_task;
static bsp_btn_cb_t s_cb;
static void *s_user;

static void emit(bsp_btn_t button, bsp_btn_ev_t event) {
    if (s_cb) s_cb(button, event, s_user);
}

static int adc_read(uint8_t command) {
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA,
        .length = 24,
        .tx_data = { command, 0, 0 },
    };
    if (spi_device_polling_transmit(s_touch, &t) != ESP_OK) return -1;
    return ((t.rx_data[1] << 8) | t.rx_data[2]) >> 3;
}

static int median(int a, int b, int c) {
    if (a > b) { int tmp = a; a = b; b = tmp; }
    if (b > c) { int tmp = b; b = c; c = tmp; }
    return a > b ? a : b;
}

static int scale_axis(int raw, int min, int max, int pixels) {
    int value = (raw - min) * (pixels - 1) / (max - min);
    return value < 0 ? 0 : (value >= pixels ? pixels - 1 : value);
}

static bool touch_read(int *x, int *y) {
    if (gpio_get_level(BSP_TOUCH_IRQ)) return false;
    int z1 = adc_read(0xB0);
    int z2 = adc_read(0xC0);
    if (z1 < 0 || z2 < 0 || z1 + 4095 - z2 < 300) return false;
    // Discard one conversion after switching axes; median rejects outliers.
    int rx[3], ry[3];
    for (int i = 0; i < 3; ++i) {
        if (adc_read(0x90) < 0) return false;
        rx[i] = adc_read(0x90);
        if (adc_read(0xD0) < 0) return false;
        ry[i] = adc_read(0xD0);
        if (rx[i] < 0 || ry[i] < 0) return false;
    }
    int raw_x = median(rx[0], rx[1], rx[2]);
    int raw_y = median(ry[0], ry[1], ry[2]);
#if CONFIG_BSP_CYD_TOUCH_DEBUG
    ESP_LOGI(TAG, "touch raw x=%d y=%d", raw_x, raw_y);
#endif
    int px = scale_axis(raw_x, CONFIG_BSP_CYD_TOUCH_X_MIN,
                        CONFIG_BSP_CYD_TOUCH_X_MAX, 4096);
    int py = scale_axis(raw_y, CONFIG_BSP_CYD_TOUCH_Y_MIN,
                        CONFIG_BSP_CYD_TOUCH_Y_MAX, 4096);
#if CONFIG_BSP_CYD_TOUCH_SWAP_XY
    int tmp = px; px = py; py = tmp;
#endif
#if CONFIG_BSP_CYD_TOUCH_INVERT_X
    px = 4095 - px;
#endif
#if CONFIG_BSP_CYD_TOUCH_INVERT_Y
    py = 4095 - py;
#endif
#if CONFIG_BSP_CYD_ROTATE_180
    px = 4095 - px;
    py = 4095 - py;
#endif
    *x = px * (BSP_LCD_W - 1) / 4095;
    *y = py * (BSP_LCD_H - 1) / 4095;
    return true;
}

static void input_task(void *arg) {
    (void)arg;
    bool touching = false, touch_long = false;
    int touch_down = 0, touch_up = 0, x0 = 0, y0 = 0, x = 0, y = 0, moved = 0;
    int64_t touch_start = 0;
    bool boot_down = false, boot_long = false, boot_pending = false;
    bool boot_candidate = false;
    int64_t boot_changed = 0, boot_start = 0, boot_released = 0;
    const int64_t long_us = BSP_BTN_LONG_PRESS_MS * 1000LL;

    for (;;) {
        int64_t now = esp_timer_get_time();
        int tx, ty;
        if (touch_read(&tx, &ty)) {
            touch_up = 0;
            x = tx; y = ty;
            if (!touching && ++touch_down >= 2) {
                touching = true;
                touch_long = false;
                x0 = x; y0 = y; moved = 0;
                touch_start = now;
                emit(BSP_BTN_OK, BSP_BTN_PRESS);
            }
            if (touching) {
                int distance = abs(x - x0) + abs(y - y0);
                if (distance > moved) moved = distance;
                if (!touch_long && moved < 24 && now - touch_start >= long_us) {
                    touch_long = true;
                    emit(BSP_BTN_OK, BSP_BTN_LONG);
                }
            }
        } else {
            touch_down = 0;
            if (touching && ++touch_up >= 2) {
                touching = false;
                if (!touch_long) {
                    int dx = x - x0, dy = y - y0;
                    if (abs(dy) >= 40 && abs(dy) > abs(dx))
                        emit(dy < 0 ? BSP_BTN_DOWN : BSP_BTN_UP, BSP_BTN_CLICK);
                    else if (moved < 24)
                        emit(BSP_BTN_OK, BSP_BTN_CLICK);
                }
                emit(BSP_BTN_OK, BSP_BTN_RELEASE);
            }
        }

        // BOOT: click=next, double click=OK, hold=back. Debounce for 40ms.
        bool raw_down = gpio_get_level(BSP_BTN_GPIO) == 0;
        if (raw_down != boot_candidate) {
            boot_candidate = raw_down;
            boot_changed = now;
        }
        if (boot_candidate != boot_down && now - boot_changed >= 40000) {
            boot_down = boot_candidate;
            if (boot_down) {
                boot_start = now;
                boot_long = false;
                emit(BSP_BTN_OK, BSP_BTN_PRESS);
            } else {
                if (!boot_long) {
                    if (boot_pending && now - boot_released <= 300000) {
                        boot_pending = false;
                        emit(BSP_BTN_OK, BSP_BTN_CLICK);
                    } else {
                        boot_pending = true;
                        boot_released = now;
                    }
                }
                // A pending single click is released after its delayed action,
                // so the first BOOT interaction only wakes a sleeping screen.
                if (!boot_pending) emit(BSP_BTN_OK, BSP_BTN_RELEASE);
            }
        }
        if (boot_down && !boot_long && now - boot_start >= long_us) {
            boot_long = true;
            boot_pending = false;
            emit(BSP_BTN_OK, BSP_BTN_LONG);
        }
        if (!boot_down && boot_pending && now - boot_released > 300000) {
            boot_pending = false;
            emit(BSP_BTN_DOWN, BSP_BTN_CLICK);
            emit(BSP_BTN_OK, BSP_BTN_RELEASE);
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

esp_err_t bsp_button_init(bsp_btn_cb_t cb, void *user) {
    if (s_task) {
        s_cb = cb; s_user = user;
        return ESP_OK;
    }
    if (CONFIG_BSP_CYD_TOUCH_X_MAX <= CONFIG_BSP_CYD_TOUCH_X_MIN ||
        CONFIG_BSP_CYD_TOUCH_Y_MAX <= CONFIG_BSP_CYD_TOUCH_Y_MIN)
        return ESP_ERR_INVALID_ARG;

    const gpio_config_t boot = {
        .pin_bit_mask = 1ULL << BSP_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    const gpio_config_t irq = {
        .pin_bit_mask = 1ULL << BSP_TOUCH_IRQ,
        .mode = GPIO_MODE_INPUT,
    };
    esp_err_t e = gpio_config(&boot);
    if (e != ESP_OK) return e;
    e = gpio_config(&irq);
    if (e != ESP_OK) return e;
    const spi_bus_config_t bus = {
        .mosi_io_num = BSP_TOUCH_MOSI, .miso_io_num = BSP_TOUCH_MISO,
        .sclk_io_num = BSP_TOUCH_SCLK, .quadwp_io_num = -1, .quadhd_io_num = -1,
        .max_transfer_sz = 3,
    };
    e = spi_bus_initialize(BSP_TOUCH_SPI_HOST, &bus, SPI_DMA_DISABLED);
    if (e != ESP_OK) return e;
    const spi_device_interface_config_t dev = {
        .clock_speed_hz = 1000000, .mode = 0,
        .spics_io_num = BSP_TOUCH_CS, .queue_size = 1,
    };
    e = spi_bus_add_device(BSP_TOUCH_SPI_HOST, &dev, &s_touch);
    if (e != ESP_OK) { spi_bus_free(BSP_TOUCH_SPI_HOST); return e; }
    s_cb = cb; s_user = user;
    if (xTaskCreate(input_task, "cyd_input", 4096, NULL, 4, &s_task) != pdPASS) {
        s_cb = NULL;
        spi_bus_remove_device(s_touch);
        s_touch = NULL;
        spi_bus_free(BSP_TOUCH_SPI_HOST);
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "CYD: swipe=UP/DOWN, tap=OK, hold=back; BOOT click/double/hold; rotation=%d",
             BSP_LCD_ROTATE_180 ? 180 : 0);
    return ESP_OK;
}

int bsp_button_read_mv(void) { return -1; }

esp_err_t bsp_button_prepare_deep_sleep(int *level) {
    if (level) *level = gpio_get_level(BSP_BTN_GPIO);
    // Continuous polling is intentional for this externally powered profile.
    return ESP_ERR_NOT_SUPPORTED;
}
