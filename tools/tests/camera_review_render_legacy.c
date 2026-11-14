// The legacy stripe path must retain a valid frame until pixels are submitted.
#include "camera_review_common.h"
#include "app_camera.h"
#include "app_camera_config.h"
#include "camera_geometry.h"
#define JSON_LIMIT 1024
#define MALLOC_CAP_DMA 1
#define MALLOC_CAP_8BIT 2
typedef void esp_tls_t;
typedef struct { int unused; } mbedtls_md5_context;
typedef unsigned UINT;
typedef uint8_t BYTE;
typedef struct { unsigned width, height, msy; void *device; } JDEC;
typedef int JRESULT;
enum { JDR_OK, JDR_MEM1, JDR_MEM2, JDR_INP };
/* @READER@ */
static int draws, ends;
static const char *scenario;
static uint16_t screen[240*135];
static bool current(uint32_t token) { return token==7; }
static bool bsp_lvgl_lock(int timeout) { return true; }
static void bsp_lvgl_unlock(void) {}
static bool bsp_lvgl_stream_set(int y,int h,uint32_t token) { return true; }
static void bsp_lvgl_stream_end(uint32_t token) { ends++; }
static bool bsp_lvgl_stream_draw(uint32_t token,int x,int y,int w,int h,const uint16_t *pixels) {
    draws++;
    if (!strcmp(scenario,"retry-geometry") && draws==1) return false;
    for (int row=0;row<h;row++) memcpy(screen+(y+row)*240+x,pixels+row*w,w*2);
    return true;
}
static void *heap_caps_calloc(size_t n,size_t size,unsigned caps) { return calloc(n,size); }
static size_t heap_caps_get_free_size(unsigned caps) { return 70000; }
static void *allocate_preview(size_t bytes) { return malloc(bytes); }
static bool present_frame(camera_reader_t *r) { assert(false); return false; }
void app_camera_snapshot(camera_state_t *out) { *out=(camera_state_t){.has_image=true}; }
static void mbedtls_md5_init(mbedtls_md5_context *ctx) {}
static void mbedtls_md5_starts(mbedtls_md5_context *ctx) {}
static void mbedtls_md5_free(mbedtls_md5_context *ctx) {}
static void mbedtls_md5_finish(mbedtls_md5_context *ctx,uint8_t *md5) { memset(md5,0,16); }
static UINT jpeg_input(JDEC *decoder,BYTE *destination,UINT requested) { return 0; }
static UINT jpeg_output(JDEC *decoder,void *bitmap,void *rect) { return 0; }
/* @DRAW@ */
static JRESULT jd_prepare(JDEC *decoder,UINT (*input)(JDEC *,BYTE *,UINT),void *work,unsigned size,void *device) {
    bool portrait=!strcmp(scenario,"retry-geometry");
    decoder->width=portrait?320:640; decoder->height=portrait?640:480;
    decoder->msy=1; decoder->device=device;
    return JDR_OK;
}
static JRESULT jd_decomp(JDEC *decoder,UINT (*output)(JDEC *,void *,void *),unsigned scale) {
    if (!strcmp(scenario,"retry-geometry")) {
        camera_reader_t *r=decoder->device;
        for (int x=0;x<r->geometry.width;x++) r->stripe[x]=0x00f8;
        for (int y=0;y<r->geometry.height;y++)
            assert(draw_stripe(r,r->geometry.x,r->geometry.y+y,r->geometry.width,1));
        return JDR_OK;
    }
    if (!strcmp(scenario,"partial")) {
        camera_reader_t *r=decoder->device;
        assert(draw_stripe(r,r->x,r->y,r->vw,1));
    }
    return JDR_INP;
}
/* @DECODE@ */
int main(int argc,char **argv) {
    assert(argc==2); scenario=argv[1];
    camera_reader_t r={.token=7,.vw=240,.vh=135};
    assert(camera_geometry_fit(640,480,r.vw,r.vh,&r.geometry));
    if (!strcmp(scenario,"retry-geometry")) {
        camera_geometry_t displayed=r.geometry;
        for (int i=0;i<240*135;i++) screen[i]=0xdead;
        assert(!decode(&r) && !r.drew && ends==0);
        assert(!memcmp(&r.geometry,&displayed,sizeof(displayed)));
        memset(r.md5_expected,'0',32); r.received=r.total=4;
        assert(decode(&r));
        for (int y=0;y<135;y++) for (int x=0;x<240;x++) {
            bool image=x>=r.geometry.x && x<r.geometry.x+r.geometry.width;
            assert(screen[y*240+x]==(image?0x00f8:0));
        }
        return 0;
    }
    if (!strcmp(scenario,"clear")) r.geometry.width=0;
    assert(!decode(&r));
    bool touched=strcmp(scenario,"untouched")!=0;
    assert(r.drew==touched && (draws>0)==touched && (ends>0)==touched);
    assert(r.stripe==NULL);
    return 0;
}
