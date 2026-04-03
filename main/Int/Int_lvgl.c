#include "Int_lvgl.h"

#define TAG "LVGL"

#define TFT_HOR_RES 240
#define TFT_VER_RES 320
#define LVGL_BUFF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * 2)

static uint8_t *lvgl_buf1 = NULL;
static uint8_t *lvgl_buf2 = NULL;
static lv_display_t * display = NULL;
static lv_obj_t * qr = NULL;
static lv_obj_t * tip_label = NULL;
static lv_obj_t * emoji_label = NULL;
static lv_obj_t * text_label = NULL;

extern const lv_font_t font_puhui_20_4;

const chatbot_emoji_t emoji_list[21] = {
    {.name = "neutral", .emoji = "😶"},
    {.name = "happy", .emoji = "🙂"},
    {.name = "laughing", .emoji = "😆"},
    {.name = "funny", .emoji = "😂"},
    {.name = "sad", .emoji = "😔"},
    {.name = "angry", .emoji = "😠"},
    {.name = "crying", .emoji = "😭"},
    {.name = "loving", .emoji = "😍"},
    {.name = "embarrassed", .emoji = "😳"},
    {.name = "surprised", .emoji = "😲"},
    {.name = "shocked", .emoji = "😱"},
    {.name = "thinking", .emoji = "🤔"},
    {.name = "winking", .emoji = "😉"},
    {.name = "cool", .emoji = "😎"},
    {.name = "relaxed", .emoji = "😌"},
    {.name = "delicious", .emoji = "🤤"},
    {.name = "kissy", .emoji = "😘"},
    {.name = "confident", .emoji = "😏"},
    {.name = "sleepy", .emoji = "😴"},
    {.name = "silly", .emoji = "😜"},
    {.name = "confused", .emoji = "🙄"}};

// 获取当前毫秒数（基于 FreeRTOS 滴答）
static uint32_t my_tick_cb(void)
{
    // xTaskGetTickCount() 返回滴答数，乘以每滴答毫秒数得到毫秒
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static void my_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    // === 关键修复：字节交换 ===
    // 因为 SPI 发送是先发高字节，而 RGB565 在内存中通常低字节在前
    // 必须把低字节挪到高字节位置发送，否则红蓝会乱跳
    lv_draw_sw_rgb565_swap(px_map, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1));
    
    /*将 px_map 写入帧缓冲或显示控制器对应区域*/
    esp_lcd_panel_draw_bitmap(panel_handle,
                              area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1,
                              px_map);

    lv_display_flush_ready(disp);
}

void display_task(void *pvParameters) {
    while(1) {
        lv_timer_handler(); // 内部自动处理锁

        // 释放 CPU 给其他任务
        DelayMs(5);      
    }
}

void Int_LVGL_Init(void)
{
    /*初始化 LVGL*/
    lv_init();

    /*设置毫秒级时钟回调，用于 LVGL 计时*/
    lv_tick_set_cb(my_tick_cb);

    /*创建缓冲区*/
    lvgl_buf1 = heap_caps_malloc(LVGL_BUFF_SIZE, MALLOC_CAP_SPIRAM);
    assert(lvgl_buf1 != NULL);
    lvgl_buf2 = heap_caps_malloc(LVGL_BUFF_SIZE, MALLOC_CAP_SPIRAM);
    assert(lvgl_buf2 != NULL);

    memset(lvgl_buf1, 0, LVGL_BUFF_SIZE);
    memset(lvgl_buf2, 0, LVGL_BUFF_SIZE);

    /*创建显示对象，用于添加屏幕和控件*/
    display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);

    lv_display_set_buffers(display, lvgl_buf1, lvgl_buf2, LVGL_BUFF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);

    /*设置刷新回调，将缓冲内容写入显示设备*/
    lv_display_set_flush_cb(display, my_flush_cb);

    /*驱动已就绪，创建 UI*/
    // 设置背景颜色
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xD4D6DC), LV_PART_MAIN);

    // 创建TIP标签
    tip_label = lv_label_create(lv_screen_active());
    // 设置标签文本
    lv_label_set_text(tip_label, "Chatbot");
    // 设置标签文字颜色
    lv_obj_set_style_text_color(tip_label, lv_color_hex(0x000000), LV_PART_MAIN);
    // 设置标签颜色
    lv_obj_set_style_bg_color(tip_label, lv_color_hex(0xffffff), LV_PART_MAIN);
    // 强制设定 label 的宽度。减去 20 是为了左右各留 10 像素的边距，防止文字贴边
    lv_obj_set_width(tip_label, TFT_HOR_RES); 
    // 设置标签透明度 不透明
    lv_obj_set_style_bg_opa(tip_label, LV_OPA_COVER,0);
    // 设置长文本模式为自动换行
    lv_label_set_long_mode(tip_label, LV_LABEL_LONG_WRAP);
    // 为这个标签挂载中文字库
    lv_obj_set_style_text_font(tip_label, &font_puhui_20_4, 0);
    // 设置对齐方式
    lv_obj_set_style_text_align(tip_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(tip_label, LV_ALIGN_TOP_MID, 0, 0);

    // 创建emoji标签
    emoji_label = lv_label_create(lv_screen_active());
    // 设置标签文本
    lv_label_set_text(emoji_label, "🙂");
    // 设置标签颜色
    lv_obj_set_style_text_color(emoji_label, lv_color_hex(0x000000), LV_PART_MAIN);
    // 强制设定 label 的宽度。减去 20 是为了左右各留 10 像素的边距，防止文字贴边
    lv_obj_set_width(emoji_label, TFT_HOR_RES - 20); 
    // 设置标签透明度 默认为 全透明
    // lv_obj_set_style_bg_opa(emoji_label, LV_OPA_COVER,0);
    // 设置长文本模式为自动换行
    lv_label_set_long_mode(emoji_label, LV_LABEL_LONG_WRAP);
    // 为这个标签挂载表情库
    lv_obj_set_style_text_font(emoji_label, font_emoji_64_init(), 0);
    // 设置对齐方式
    lv_obj_set_style_text_align(emoji_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(emoji_label, LV_ALIGN_CENTER, 0, -70);

    // 创建text标签
    text_label = lv_label_create(lv_screen_active());
    // 设置标签文本
    lv_label_set_text(text_label, "你好，世界");
    // 设置标签颜色
    lv_obj_set_style_text_color(text_label, lv_color_hex(0x000000), LV_PART_MAIN);
    // 强制设定 label 的宽度。减去 20 是为了左右各留 10 像素的边距，防止文字贴边
    lv_obj_set_width(text_label, TFT_HOR_RES - 20); 
    // 设置标签透明度 默认为 全透明
    // lv_obj_set_style_bg_opa(text_label, LV_OPA_COVER,0);
    // 设置长文本模式为自动换行
    lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
    // 为这个标签挂载中文字库
    lv_obj_set_style_text_font(text_label, &font_puhui_20_4, 0);
    // 设置对齐方式
    lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align_to(text_label, emoji_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);

    xTaskCreate(display_task, "display_task", 8192, NULL, 2, NULL);
}

void Int_LVGL_Qrcode(char *data,uint32_t len)
{
    lv_lock();

    lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);

    qr = lv_qrcode_create(lv_screen_active());
    lv_qrcode_set_size(qr, 200);
    lv_qrcode_set_dark_color(qr, fg_color);
    lv_qrcode_set_light_color(qr, bg_color);

    /*Set data*/
    lv_qrcode_update(qr, data, len);
    lv_obj_center(qr);

    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, bg_color, 0);
    lv_obj_set_style_border_width(qr, 5, 0);  
    
    lv_unlock();
}

void Int_LVGL_DeleteQr()
{
    lv_lock();

    if (qr != NULL) {
        lv_obj_del(qr);
    }

    lv_unlock();
}

void Int_LVGL_ShowTip(char *text)
{
    lv_lock();  
    lv_label_set_text(tip_label, text);
    lv_unlock();  
}

void Int_LVGL_ShowEmoji(char *emoji_name)
{
    lv_lock(); 

    bool isfound = false;
    for (uint8_t i = 0; i < 21; i++)
    {
        if (strcmp(emoji_list[i].name, emoji_name) == 0)
        {
            lv_label_set_text(emoji_label, emoji_list[i].emoji);
            isfound = true;
            break;
        }
    }
    if (isfound == false)
    {
        lv_label_set_text(emoji_label, "😏");
    }

    lv_unlock();  
}

void Int_LVGL_ShowText(char *text)
{
    lv_lock();  
    lv_label_set_text(text_label, text);
    lv_unlock();  
}
