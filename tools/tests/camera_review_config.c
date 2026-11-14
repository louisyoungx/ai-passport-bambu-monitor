// Compile the real loader to keep missing configuration distinct from lost trust.
#include "camera_review_common.h"
#include "app_camera_config.h"
typedef int nvs_handle_t;
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NVS_NOT_FOUND 1
#define NVS_READONLY 0
static const char *scenario;
static int closes;
static int mbedtls_sha256(const uint8_t *input,size_t size,uint8_t *out,int sha224) { memset(out,0x42,32); return 0; }
static int nvs_flash_init(void) { return !strcmp(scenario,"init")?ESP_FAIL:ESP_OK; }
static int nvs_open(const char *name,int mode,nvs_handle_t *handle) {
    *handle=1;
    return !strcmp(scenario,"absent")?ESP_ERR_NVS_NOT_FOUND:!strcmp(scenario,"open")?ESP_FAIL:ESP_OK;
}
static int nvs_get_str(nvs_handle_t handle,const char *key,char *out,size_t *size) {
    if (!strcmp(scenario,key)) return ESP_FAIL;
    const char *value=!strcmp(key,"serial")?(!strcmp(scenario,"identity")?"OTHER":"TEST-PRINTER"):
                      !strcmp(key,"host")?"printer.test":!strcmp(key,"code")?"TEST1234":"Printer";
    size_t n=strlen(value)+1; assert(*size>=n); memcpy(out,value,n); *size=n; return ESP_OK;
}
static int nvs_get_blob(nvs_handle_t handle,const char *key,void *out,size_t *size) {
    if (!strcmp(scenario,"cert")) return ESP_FAIL;
    if (!strcmp(scenario,"missing-cert")) return ESP_ERR_NVS_NOT_FOUND;
    *size=!strcmp(scenario,"short-cert")?31:32; memset(out,0x42,*size); return ESP_OK;
}
static int nvs_get_u8(nvs_handle_t handle,const char *key,uint8_t *out) {
    *out=!strcmp(scenario,"invalid-mode")?3:1;
    return !strcmp(scenario,"mode")?ESP_FAIL:ESP_OK;
}
static void nvs_close(nvs_handle_t handle) { closes++; }
/* @NAMESPACE@ */
/* @LOAD@ */
int main(int argc,char **argv) {
    assert(argc==2); scenario=argv[1];
    camera_config_t camera; memset(&camera,0xa5,sizeof(camera));
    bool ok=camera_config_load("TEST-PRINTER",&camera);
    bool valid=!strcmp(scenario,"valid"), absent=!strcmp(scenario,"absent");
    assert(ok==(valid||absent));
    if (valid) {
        uint8_t pin[32]; memset(pin,0x42,32);
        assert(!strcmp(camera.host,"printer.test") && !strcmp(camera.access_code,"TEST1234"));
        assert(!memcmp(camera.certificate_sha256,pin,32) && camera.transport==1);
    } else {
        camera_config_t empty={0}; assert(!memcmp(&camera,&empty,sizeof(camera)));
    }
    bool opened=strcmp(scenario,"init") && strcmp(scenario,"absent") && strcmp(scenario,"open");
    assert(closes==opened);
    return 0;
}
