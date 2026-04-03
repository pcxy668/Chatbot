#include <stdio.h>
#include "Int_button.h"
#include "Int_wifi.h"
#include "Int_lvgl.h"
#include "Int_activate.h"
#include "Int_sr.h"
#include "Int_opus_encoder.h"
#include "Int_opus_decoder.h"
#include "Int_websocket.h"
#include "Com_utils.h"
 
void app_main(void)
{
    // ================== 外设初始化 ==================
    Int_LCD_Init();
    Int_LVGL_Init();
    Int_Button_Init();
    Int_WIFI_Init();
    Int_Activate_Request_With_Https();
    Int_Sr_Init();
    Int_Opus_Encoder_Init();
    Int_Opus_Decoder_Init();
    Int_Websocket_Init();

    // ================== 堆内存检测 ================== 
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    printf("Free internal heap: %d bytes\n", free_heap);

    // ================== 任务调度 ================== 
}
