#ifndef __INT_WEBSOCKET_H__
#define __INT_WEBSOCKET_H__

#include "esp_log.h"
#include "esp_websocket_client.h"
#include "Int_activate.h"
#include "Int_lvgl.h"
#include "cJSON.h"

#define WS_EVENT_BIT BIT0
#define LCD_EVENT_BIT BIT1

void Int_Websocket_Init(void);
void Int_Websocket_Start(void);
void Int_Websocket_Close(void);
bool Int_Websocket_IsConnected(void);
void Int_Websocket_StartListen(void);
void Int_Websocket_StopListen(void);

#endif /* __INT_WEBSOCKET_H__ */
