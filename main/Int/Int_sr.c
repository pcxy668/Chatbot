#include "Int_sr.h"

#define TAG "INT-SR"

static const esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
bool isFirstCount = true;
TickType_t start = 0;

void vad_change_cb(void)
{
    // 向websocket发送开始监听帧
    if (chatbot_handle.cur_vad_state == VAD_SPEECH && chatbot_handle.state == CHATBOT_IDLE && Int_Websocket_IsConnected())
    {
        Int_LVGL_ShowTip("正在讲话……");
        ESP_LOGI(TAG, "开始监听");
        chatbot_handle.state = CHATBOT_LISTENING;
        isFirstCount = true;
        Int_Websocket_StartListen();
    }
    // 向websocket发送停止监听帧
    else if (chatbot_handle.cur_vad_state == VAD_SILENCE && chatbot_handle.state == CHATBOT_LISTENING && Int_Websocket_IsConnected())
    {
        Int_Websocket_StopListen();
        chatbot_handle.state = CHATBOT_IDLE;
        ESP_LOGI(TAG, "停止监听");
    }
}

void wake_cb(void)
{
    if (!Int_Websocket_IsConnected())
    {
        Int_LVGL_ShowTip("正在唤醒……");
        Int_Websocket_Start();
    }
}

void feed_task(void *arg)
{
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int16_t *feed_buff = malloc(audio_chunksize * sizeof(int16_t) * nch);
    assert(feed_buff);
    while (1) 
    {
        Int_Hard_Codec_Record((uint8_t *)feed_buff, audio_chunksize * sizeof(int16_t) * nch);
        afe_handle->feed(afe_data, feed_buff);
    }
}

void detect_task(void *arg)
{
    // modify wakenet detection threshold
    // afe_handle->set_wakenet_threshold(afe_data, 1, 0.6); // set model1's threshold to 0.6
    // afe_handle->set_wakenet_threshold(afe_data, 2, 0.6); // set model2's threshold to 0.6
    // afe_handle->reset_wakenet_threshold(afe_data, 1);    // reset model1's threshold to default
    // afe_handle->reset_wakenet_threshold(afe_data, 2);    // reset model2's threshold to default
    afe_fetch_result_t* res = NULL;
    while (1)
    {
        res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            continue;
        }
        vad_state_t vad_state = res->vad_state;
        wakenet_state_t wakeup_state = res->wakeup_state;
        int16_t *processed_audio = res->data;
        int aduio_size = res->data_size;
        int vad_cache_size = res->vad_cache_size;

        if (wakeup_state == WAKENET_DETECTED)
        {
            ESP_LOGI(TAG, "唤醒喵喵!");
            chatbot_handle.last_vad_state = false;
            chatbot_handle.wakeup_state = true;
            if (chatbot_handle.wake_cb != NULL) {
                chatbot_handle.wake_cb();
            }
        }
        
        if (chatbot_handle.wakeup_state == true) {
            chatbot_handle.cur_vad_state = (vad_state == VAD_SPEECH ? true : false);

            // 人声状态改变时，执行回调函数
            if (chatbot_handle.cur_vad_state != chatbot_handle.last_vad_state)
            {
                chatbot_handle.last_vad_state = chatbot_handle.cur_vad_state;
                if (chatbot_handle.vad_changed_cb != NULL)
                {
                    chatbot_handle.vad_changed_cb();
                }
            }
        }

        // 如果chatbot是监听状态，将音频数据发送到编码环形缓冲区
        if (chatbot_handle.state == CHATBOT_LISTENING)
        {
            if (vad_cache_size > 0)
            {
                int16_t *vad_cache = res->vad_cache;
                xRingbufferSend(chatbot_handle.sr2encoder_rb, vad_cache, vad_cache_size, portMAX_DELAY);
            }
            xRingbufferSend(chatbot_handle.sr2encoder_rb, processed_audio, aduio_size, portMAX_DELAY);
        }
    }
}

void Int_Sr_Init(void)
{
    // 初始化硬件编解码器ES8311
    Int_Hard_Codec_Init();

    // 初始化AFE配置
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);

    afe_config->aec_init = false; // 禁用回声消除
    afe_config->se_init = false; // 禁用人生增强
    afe_config->ns_init = false; // 禁用噪音消除
    afe_config->wakenet_init = true; // 启用WakeNet
    afe_config->wakenet_mode = DET_MODE_95; // 唤醒模式
    afe_config->vad_init = true; // 启用VAD
    afe_config->vad_min_noise_ms = 500;  // The minimum duration of noise or silence in ms.
    afe_config->vad_min_speech_ms = 128;  // The minimum duration of speech in ms.
    afe_config->vad_mode = VAD_MODE_1;    // The larger the mode, the higher the speech trigger probability.
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM; // 内存分配模式

    // 创建AFE实例 
    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    // 注册afe状态回调函数
    chatbot_handle.vad_changed_cb = vad_change_cb;
    chatbot_handle.wake_cb = wake_cb;

    // 启动输入数据和获取数据的任务
    xTaskCreatePinnedToCoreWithCaps(feed_task, "feed_task", 8 * 1024, NULL, 3, NULL, 0, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(detect_task, "detect_task", 4 * 1024, NULL, 3, NULL, 1, MALLOC_CAP_SPIRAM);
}
