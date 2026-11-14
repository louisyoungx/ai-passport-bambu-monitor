#include "camera_review_common.h"
#include "app_config.h"
#include "app_camera_config.h"
#include "app_camera.h"
#include "camera_geometry.h"
#define JSON_LIMIT 1024
typedef void esp_tls_t;
typedef struct { int unused; } mbedtls_md5_context;
/* @READER@ */
typedef struct { size_t len; const uint8_t *p; } certificate_bytes_t;
typedef struct certificate_name { certificate_bytes_t oid,val; struct certificate_name *next; } mbedtls_x509_name;
typedef struct { mbedtls_x509_name subject; certificate_bytes_t raw; } mbedtls_x509_crt;
#define MBEDTLS_OID_AT_CN "CN"
#define MBEDTLS_X509_BADCERT_NOT_TRUSTED 1
static int mbedtls_sha256(const uint8_t *input,size_t size,uint8_t *output,int sha224) {
    assert(size==32); memcpy(output,input,32); return 0;
}
/* @VERIFY@ */
static device_config_t nvs_config;
static bool load_ok = true;
static const char *legacy_serial = "TESTLEGACY00001";
#define CFG_BAMBU_SERIAL legacy_serial
bool config_load(device_config_t *out) { *out = nvs_config; return load_ok; }
/* @ROSTER@ */
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define SBUF 1400
static char s_buf[SBUF], saved_serial[34];
static camera_config_t saved_camera;
static camera_config_t loaded_camera;
static bool paired;
static bool camera_load_ok=true, camera_save_ok=true;
static int saves, restarts;
typedef struct { int content_len; const char *body; char response[SBUF]; } httpd_req_t;
static int read_body(httpd_req_t *req, char *out, int size) { strlcpy(out,req->body,size); return req->content_len; }
static void send_json(httpd_req_t *req, const char *json) { strlcpy(req->response,json,sizeof(req->response)); }
static void schedule_restart(int ms) { restarts++; }
bool camera_config_load(const char *serial, camera_config_t *out) {
    *out=loaded_camera;
    if (!camera_load_ok) { memset(out,0,sizeof(*out)); return false; }
    return true;
}
bool camera_config_save(const char *serial, const camera_config_t *cfg) {
    saves++;
    if (!camera_save_ok) return false;
    strlcpy(saved_serial,serial,sizeof(saved_serial)); saved_camera=*cfg; return true;
}
/* @HELPERS@ */
/* @GET@ */
/* @POST@ */
int main(int argc, char **argv) {
    assert(argc==2); const char *scenario=argv[1];
    if (!strncmp(scenario,"pin-",4)) {
        paired=true;
        strlcpy(loaded_camera.host,"printer.test",sizeof(loaded_camera.host));
        strlcpy(loaded_camera.access_code,"TEST1234",sizeof(loaded_camera.access_code));
        loaded_camera.transport=1;
        memset(loaded_camera.certificate_sha256,0x42,32);
        const char *host="printer.test", *code="", *model="", *mode="1", *pair="pair=0";
        if (!strcmp(scenario,"pin-host")) host="192.168.1.99";
        if (!strcmp(scenario,"pin-code")) code="NEW12345";
        if (!strcmp(scenario,"pin-disable")) host="";
        if (!strcmp(scenario,"pin-enable")) loaded_camera.host[0]=0;
        if (!strcmp(scenario,"pin-model")) model="Renamed+Printer";
        if (!strcmp(scenario,"pin-mode")) mode="2";
        bool repair=!strcmp(scenario,"pin-repair") || !strcmp(scenario,"pin-repair-encoded");
        if (!strcmp(scenario,"pin-repair")) pair="pair=1";
        if (!strcmp(scenario,"pin-repair-encoded")) pair="pair=%31";
        if (!strcmp(scenario,"pin-repair-prefix")) pair="notpair=1";
        if (!strcmp(scenario,"pin-repair-suffix")) pair="repair=1";
        if (!strcmp(scenario,"pin-repair-value")) pair="pair=10";
        if (!strcmp(scenario,"pin-repair-nul")) pair="pair=1%00";
        if (!strcmp(scenario,"pin-repair-encoded-nul")) pair="pair=%31%00";
        if (!strcmp(scenario,"pin-omitted")) pair="other=0";
        if (!strcmp(scenario,"pin-empty")) pair="pair=";
        if (!strcmp(scenario,"pin-load-error")) { camera_load_ok=false; code="NEW12345"; }
        if (!strcmp(scenario,"pin-save-error")) camera_save_ok=false;
        char body[256];
        snprintf(body,sizeof(body),"idx=0&host=%s&code=%s&model=%s&mode=%s&%s",host,code,model,mode,pair);
        httpd_req_t req={.content_len=(int)strlen(body),.body=body};
        assert(camera_save_handler(&req)==ESP_OK);
        if (!camera_load_ok || !camera_save_ok) {
            assert(strstr(req.response,"\"err\"") && restarts==0);
            assert(saves==(camera_load_ok?1:0));
            assert(!saved_serial[0]); return 0;
        }
        assert(!strcmp(req.response,"{\"ok\":1}"));
        assert(saves==1 && restarts==1 && !strcmp(saved_serial,legacy_serial));
        uint8_t zero[32]={0};
        assert(!memcmp(saved_camera.certificate_sha256,repair?zero:loaded_camera.certificate_sha256,32));
        assert(!strcmp(saved_camera.host,host) && !strcmp(saved_camera.access_code,code[0]?code:"TEST1234"));
        assert(saved_camera.transport==mode[0]-'0');
        if (model[0]) assert(!strcmp(saved_camera.model,"Renamed Printer"));
        // The saved configuration must still reject a changed leaf with the
        // correct serial; explicit re-pair alone restores first-pair behavior.
        camera_reader_t reader={.config=saved_camera};
        strlcpy(reader.serial,legacy_serial,sizeof(reader.serial));
        uint8_t leaf[32]; memset(leaf,0x42,32);
        mbedtls_x509_crt cert={.subject={.oid={2,(const uint8_t *)"CN"},
            .val={strlen(legacy_serial),(const uint8_t *)legacy_serial}},.raw={32,leaf}};
        uint32_t flags=0;
        assert(verify_certificate(&reader,&cert,0,&flags)==0 && !flags && !reader.identity_failed);
        memset(leaf,0x43,32);
        assert(verify_certificate(&reader,&cert,0,&flags)==0 && reader.identity_failed==!repair);
        assert((flags!=0)==!repair);
        return 0;
    }
    int index=0; const char *expected=legacy_serial;
    if (!strcmp(scenario,"nvs") || !strcmp(scenario,"sparse")) {
        index=!strcmp(scenario,"sparse") ? 2 : 0; expected="TESTNVS00002";
        strlcpy(nvs_config.bambu_sn[index],expected,sizeof(nvs_config.bambu_sn[index]));
        strlcpy(nvs_config.bambu_name[index],"Printer A",sizeof(nvs_config.bambu_name[index]));
    } else if (!strcmp(scenario,"placeholder")) {
        device_config_t effective;
        legacy_serial="00C00AAAA000000"; assert(config_load_printers(&effective));
        assert(config_machine_count(&effective)==0);
        legacy_serial=""; assert(config_load_printers(&effective)); assert(config_machine_count(&effective)==0);
        httpd_req_t req={0}; assert(camera_get_handler(&req)==ESP_OK);
        assert(!strcmp(req.response,"{\"machines\":[]}")); return 0;
    } else if (!strcmp(scenario,"load-error")) load_ok=false;
    device_config_t effective;
    assert(config_load_printers(&effective)==load_ok);
    assert(!strcmp(effective.bambu_sn[index],expected));
    assert(index==0 || !effective.bambu_sn[0][0]);
    httpd_req_t get={0}; assert(camera_get_handler(&get)==ESP_OK);
    char selected[24]; snprintf(selected,sizeof(selected),"\"index\":%d",index);
    assert(strstr(get.response,selected)); assert(!strstr(get.response,"TEST1234"));
    assert(!strstr(get.response,"\"cert\"") && !strstr(get.response,"\"code\""));
    char body[160]; snprintf(body,sizeof(body),"idx=%d&host=printer.test&code=TEST1234&model=Test&mode=1&pair=0",index);
    httpd_req_t post={.content_len=(int)strlen(body),.body=body};
    assert(camera_save_handler(&post)==ESP_OK && !strcmp(post.response,"{\"ok\":1}"));
    assert(saves==1 && restarts==1 && !strcmp(saved_serial,expected));
    assert(!strcmp(saved_camera.access_code,"TEST1234") && !strcmp(saved_camera.host,"printer.test"));
    if (!strcmp(scenario,"legacy")) assert(config_machine_count(&nvs_config)==0); // No account migration/write.
    return 0;
}
