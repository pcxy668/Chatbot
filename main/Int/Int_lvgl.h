#ifndef __INT_LVGL_H__
#define __INT_LVGL_H__

#include "Int_lcd.h"
#include "lvgl.h"
#include "Com_utils.h"
#include "Com_types.h"
#include "font_emoji.h"

void Int_LVGL_Init(void);
void Int_LVGL_Qrcode(char *data,uint32_t len);
void Int_LVGL_DeleteQr();
void Int_LVGL_ShowTip(char *text);
void Int_LVGL_ShowEmoji(char *emoji_name);
void Int_LVGL_ShowText(char *text);

#endif /* __INT_LVGL_H__ */
