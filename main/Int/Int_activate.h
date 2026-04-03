#ifndef __INT_ACTIVATE_H__
#define __INT_ACTIVATE_H__

#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_app_desc.h"
#include "nvs_flash.h"
#include "stdlib.h"
#include "cJSON.h"
#include "Com_types.h"
#include "Int_lvgl.h"
#include "freertos/ringbuf.h"

extern chatbot_handle_t chatbot_handle;

void Int_Activate_Request_With_Https(void);

#endif /* __INT_ACTIVATE_H__ */
