#include "Int_websocket.h"

esp_websocket_client_handle_t ws_client;


static const char *TAG = "websocket";

static void websocket_send_hello(void)
{
    esp_websocket_client_send_text(ws_client,
                                   "{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\","
                                   "\"features\":{\"mcp\":true},\"audio_params\":{\"format\":\"opus\","
                                   "\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}", 
                                   162,
                                   1000);
}

static void websocket_send_wakeup(void)
{
    esp_websocket_client_send_text(ws_client,
                                   "{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"哈喽\"}", 
                                   50,
                                   1000);
}

static void websocket_parseText(char *data, int len)
{
    cJSON *root = cJSON_ParseWithLength(data, len);
    if (root == NULL) 
    { 
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (type == NULL)
    {
        return;
    }

    if (strcmp(type->valuestring, "hello") == 0)
    {
        websocket_send_wakeup();
    }
    else if (strcmp(type->valuestring, "tts") == 0)
    {
        cJSON *state = cJSON_GetObjectItem(root, "state");
        if (strcmp(state->valuestring, "sentence_start") == 0)
        {
            cJSON *text = cJSON_GetObjectItem(root, "text");
            Int_LVGL_ShowText(text->valuestring);
        }
        else if (strcmp(state->valuestring, "start") == 0)
        {
            Int_LVGL_ShowTip("正在思考……");
            chatbot_handle.state = CHATBOT_SPEAKING;
        }
        else if (strcmp(state->valuestring, "stop") == 0)
        {
            chatbot_handle.state = CHATBOT_IDLE;
            Int_LVGL_ShowTip("请讲话……");
        }
    }
    else if (strcmp(type->valuestring, "llm") == 0)
    {
        cJSON *emotion = cJSON_GetObjectItem(root, "emotion");
        Int_LVGL_ShowEmoji(emotion->valuestring);
    }

    cJSON_Delete(root);
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
    case WEBSOCKET_EVENT_BEGIN:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_BEGIN");
        chatbot_handle.state = CHATBOT_IDLE;
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        Int_LVGL_ShowTip("已连接");
        websocket_send_hello();
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        Int_LVGL_ShowTip("请重新唤醒");
        chatbot_handle.state = CHATBOT_IDLE;
        chatbot_handle.wakeup_state = false;
        break;
    case WEBSOCKET_EVENT_DATA:
        // ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        // ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        if (data->op_code == 0x01) 
        { 
            websocket_parseText(data->data_ptr, data->data_len);
        } 
        else if (data->op_code == 0x02) 
        { 
            // Opcode 0x2 indicates binary data
            xRingbufferSend(chatbot_handle.ws2decoder_rb, data->data_ptr, data->data_len, 1000);
        }
        else if (data->op_code == 0x08 && data->data_len == 2)
        {
            ESP_LOGW(TAG, "Received closed message with code=%d", 256 * data->data_ptr[0] + data->data_ptr[1]);
        } 
        else 
        {
            ESP_LOGW(TAG, "Received=%.*s\n\n", data->data_len, (char *)data->data_ptr);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    case WEBSOCKET_EVENT_FINISH:
        Int_LVGL_ShowTip("请重新唤醒");
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_FINISH");
        chatbot_handle.state = CHATBOT_IDLE;
        chatbot_handle.wakeup_state = false;
        break;
    }
}

void pushOpus_task(void *pvParameters)
{
    while (1)
    {
        size_t size = 0;
        void * start = xRingbufferReceive(chatbot_handle.encoder2ws_rb, &size, portMAX_DELAY);
        esp_websocket_client_send_bin(ws_client, start, size, 1000);
        vRingbufferReturnItem(chatbot_handle.encoder2ws_rb, start);
    }
}

void Int_Websocket_Init(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");

    esp_websocket_client_config_t websocket_cfg = {};

    websocket_cfg.uri = chatbot_handle.ws_url;
    websocket_cfg.transport = WEBSOCKET_TRANSPORT_OVER_SSL;
    websocket_cfg.crt_bundle_attach = esp_crt_bundle_attach;
    websocket_cfg.reconnect_timeout_ms = 3000;
    ws_client = esp_websocket_client_init(&websocket_cfg);

    char auth[30] = {0};
    sprintf(auth, "Bearer %s", chatbot_handle.ws_token);
    esp_websocket_client_append_header(ws_client, "Authorization", auth);
    esp_websocket_client_append_header(ws_client, "Protocol-Version", "1");
    esp_websocket_client_append_header(ws_client, "Device-Id", chatbot_handle.mac_str);
    esp_websocket_client_append_header(ws_client, "Client-Id", chatbot_handle.uuid);

    esp_websocket_register_events(ws_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, NULL);    

    xTaskCreatePinnedToCoreWithCaps(pushOpus_task, "pushOpus_task", 8 * 1024, NULL, 3, NULL, 1, MALLOC_CAP_SPIRAM);

    Int_LVGL_ShowTip("准备就绪");
}

void Int_Websocket_Start(void)
{
    esp_websocket_client_start(ws_client);
}

void Int_Websocket_Close(void)
{
    esp_websocket_client_close(ws_client,1000);
}

bool Int_Websocket_IsConnected(void)
{
    return esp_websocket_client_is_connected(ws_client);
}

void Int_Websocket_StartListen(void)
{
    esp_websocket_client_send_text(ws_client,
                                   "{\"type\":\"listen\",\"state\":\"start\",\"mode\":\"manual\"}", 
                                   49,
                                   1000);
}

void Int_Websocket_StopListen(void)
{
    esp_websocket_client_send_text(ws_client,
                                   "{\"type\":\"listen\",\"state\":\"stop\"}", 
                                   32,
                                   1000);
}
