// CYD is externally powered and has no CW2017 battery gauge.
#include "bsp_battery.h"

esp_err_t bsp_battery_init(void) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t bsp_battery_sleep(void) { return ESP_OK; }
int bsp_battery_soc(void) { return -1; }
int bsp_battery_mv(void) { return -1; }
// The CH340 supplies UART, not native USB state; there is no charge sensor.
bool bsp_battery_usb_connected(void) { return false; }
