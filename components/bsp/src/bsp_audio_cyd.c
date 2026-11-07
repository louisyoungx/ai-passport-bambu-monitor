// ESP32 internal DAC channel 1 (GPIO26) feeds the CYD speaker amplifier.
#include "bsp_audio.h"
#include "driver/dac_continuous.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DAC_DESC_COUNT 4
#define DAC_BUFFER_BYTES 512

static dac_continuous_handle_t s_dac;
static uint32_t s_hz;
static uint8_t s_volume = 50;

esp_err_t bsp_audio_init(void) { return ESP_OK; }

esp_err_t bsp_audio_close(void) {
    if (!s_dac) return ESP_OK;
    // Synchronous writes finish when queued, not when played. Allow the
    // entire 16-bit-aligned DMA ring to drain before disabling the DAC.
    if (s_hz) {
        uint32_t samples = DAC_DESC_COUNT * DAC_BUFFER_BYTES / 2;
        vTaskDelay(pdMS_TO_TICKS((samples * 1000 + s_hz - 1) / s_hz + 2));
    }
    esp_err_t e = dac_continuous_disable(s_dac);
    if (e != ESP_OK) return e;
    e = dac_continuous_del_channels(s_dac);
    if (e == ESP_OK) { s_dac = NULL; s_hz = 0; }
    return e;
}

esp_err_t bsp_audio_set_format(uint32_t hz, uint8_t bits, uint8_t ch) {
    if (bits != 16 || ch != 1 || hz < 8000 || hz > 48000)
        return ESP_ERR_NOT_SUPPORTED;
    if (s_dac && s_hz == hz) return ESP_OK;
    esp_err_t e = bsp_audio_close();
    if (e != ESP_OK) return e;
    const dac_continuous_config_t cfg = {
        .chan_mask = DAC_CHANNEL_MASK_CH1, // GPIO26; leave GPIO25 to touch SPI.
        .desc_num = DAC_DESC_COUNT,
        .buf_size = DAC_BUFFER_BYTES,
        .freq_hz = hz,
        .offset = 0,
        .clk_src = DAC_DIGI_CLK_SRC_APLL, // 16kHz is below PLL_D2's minimum.
        .chan_mode = DAC_CHANNEL_MODE_SIMUL,
    };
    e = dac_continuous_new_channels(&cfg, &s_dac);
    if (e != ESP_OK) return e;
    e = dac_continuous_enable(s_dac);
    if (e != ESP_OK) {
        dac_continuous_del_channels(s_dac);
        s_dac = NULL;
        return e;
    }
    s_hz = hz;
    return ESP_OK;
}

esp_err_t bsp_audio_write(const void *pcm, size_t bytes) {
    if (!s_dac) return ESP_ERR_INVALID_STATE;
    if (!pcm || bytes % sizeof(int16_t)) return ESP_ERR_INVALID_ARG;
    const int16_t *samples = pcm;
    size_t count = bytes / sizeof(*samples);
    uint8_t converted[256];
    while (count) {
        size_t n = count < sizeof(converted) ? count : sizeof(converted);
        for (size_t i = 0; i < n; ++i)
            converted[i] = (uint8_t)(128 + (int32_t)samples[i] * s_volume / 256 / 100);
        size_t written = 0;
        esp_err_t e = dac_continuous_write(s_dac, converted, n, &written, 1000);
        if (e != ESP_OK) return e;
        if (written != n) return ESP_ERR_TIMEOUT;
        samples += n;
        count -= n;
    }
    return ESP_OK;
}

esp_err_t bsp_audio_read(void *pcm, size_t bytes) {
    (void)pcm;
    (void)bytes;
    return ESP_ERR_NOT_SUPPORTED; // CYD has no microphone/codec.
}

void bsp_audio_set_volume(uint8_t percent) {
    s_volume = percent > 100 ? 100 : percent;
}
