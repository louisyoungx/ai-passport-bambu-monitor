#include "app_camera_config.h"
#include <stdio.h>
#include <string.h>
#include "mbedtls/sha256.h"
#include "nvs.h"
#include "nvs_flash.h"

static bool namespace_for(const char *serial, char name[16]) {
    if (!serial || !serial[0] || strlen(serial) > 33) return false;
    uint8_t hash[32];
    if (mbedtls_sha256((const uint8_t *)serial, strlen(serial), hash, 0)) return false;
    name[0] = 'c';
    for (int i = 0; i < 7; i++) snprintf(name + 1 + i * 2, 3, "%02x", hash[i]);
    return true;
}

bool camera_config_load(const char *serial, camera_config_t *out) {
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    char name[16], stored[34] = {0};
    if (!namespace_for(serial, name) || nvs_flash_init() != ESP_OK) return false;
    nvs_handle_t handle;
    esp_err_t opened = nvs_open(name, NVS_READONLY, &handle);
    if (opened == ESP_ERR_NVS_NOT_FOUND) return true; // A new printer has no saved trust yet.
    if (opened != ESP_OK) return false;
    camera_config_t loaded = {0};
    size_t n = sizeof(stored);
    bool ok = nvs_get_str(handle, "serial", stored, &n) == ESP_OK && !strcmp(stored, serial);
    if (ok) {
        n = sizeof(loaded.host); ok &= nvs_get_str(handle, "host", loaded.host, &n) == ESP_OK;
        n = sizeof(loaded.access_code); ok &= nvs_get_str(handle, "code", loaded.access_code, &n) == ESP_OK;
        n = sizeof(loaded.model); ok &= nvs_get_str(handle, "model", loaded.model, &n) == ESP_OK;
        n = sizeof(loaded.certificate_sha256);
        ok &= nvs_get_blob(handle, "cert", loaded.certificate_sha256, &n) == ESP_OK &&
              n == sizeof(loaded.certificate_sha256);
        ok &= nvs_get_u8(handle, "transport", &loaded.transport) == ESP_OK &&
              loaded.transport <= CAMERA_JPEG_STREAM;
    }
    nvs_close(handle);
    // Never expose a partially read host/code with a missing certificate pin.
    if (ok) *out = loaded;
    return ok;
}

bool camera_config_save(const char *serial, const camera_config_t *config) {
    if (!config || config->transport > CAMERA_JPEG_STREAM ||
        !memchr(config->host, 0, sizeof(config->host)) ||
        !memchr(config->model, 0, sizeof(config->model)) ||
        !memchr(config->access_code, 0, sizeof(config->access_code))) return false;
    if (config->host[0] && strlen(config->access_code) != 8) return false;
    char name[16], stored[34] = {0};
    if (!namespace_for(serial, name) || nvs_flash_init() != ESP_OK) return false;
    nvs_handle_t handle;
    if (nvs_open(name, NVS_READWRITE, &handle) != ESP_OK) return false;
    size_t n = sizeof(stored);
    esp_err_t found = nvs_get_str(handle, "serial", stored, &n);
    bool ok = found == ESP_ERR_NVS_NOT_FOUND || (found == ESP_OK && !strcmp(stored, serial));
    if (ok) {
        ok &= nvs_set_str(handle, "serial", serial) == ESP_OK;
        ok &= nvs_set_str(handle, "host", config->host) == ESP_OK;
        ok &= nvs_set_str(handle, "code", config->access_code) == ESP_OK;
        ok &= nvs_set_str(handle, "model", config->model) == ESP_OK;
        ok &= nvs_set_u8(handle, "transport", config->transport) == ESP_OK;
        ok &= nvs_set_blob(handle, "cert", config->certificate_sha256, sizeof(config->certificate_sha256)) == ESP_OK;
        if (ok) ok = nvs_commit(handle) == ESP_OK;
    }
    nvs_close(handle);
    return ok;
}
