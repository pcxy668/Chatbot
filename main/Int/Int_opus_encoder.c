#include "Int_opus_encoder.h"

#define TAG "Opus-Encoder"

esp_audio_enc_handle_t encoder = NULL;

void encoder_task(void *pvParameters);

void Int_Opus_Encoder_Init(void)
{
    // 配置Opus编码器
    esp_opus_enc_config_t opus_cfg = {
    .sample_rate        = ESP_AUDIO_SAMPLE_RATE_16K,
    .channel            = ESP_AUDIO_MONO, 
    .bits_per_sample    = ESP_AUDIO_BIT16, 
    .bitrate            = 32000,     
    .frame_duration     = ESP_OPUS_ENC_FRAME_DURATION_60_MS, 
    .application_mode   = ESP_OPUS_ENC_APPLICATION_AUDIO,
    .complexity         = 5, 
    .enable_fec         = false,
    .enable_dtx         = false, 
    .enable_vbr         = false                         
    };

    esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &encoder);

    xTaskCreatePinnedToCoreWithCaps(encoder_task, "encoder_task", 32 * 1024, NULL, 3, NULL, 1, MALLOC_CAP_SPIRAM);
}

void encoder_task(void *pvParameters)
{
    // 获取编码器帧大小
    int pcm_size = 0, raw_size = 0;
    esp_opus_enc_get_frame_size(encoder, &pcm_size, &raw_size);
    uint8_t *pcm_data = heap_caps_malloc(pcm_size, MALLOC_CAP_SPIRAM);
    uint8_t *raw_data = heap_caps_malloc(raw_size, MALLOC_CAP_SPIRAM);

    // 配置输入输出帧
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = pcm_data,
        .len = pcm_size,
    };
    esp_audio_enc_out_frame_t out_frame = {
        .buffer = raw_data,
        .len = raw_size,
    };

    while (1)
    {
        size_t remain_size = pcm_size;
        size_t cur_size = 0;

        // 获取环形缓冲区一个完整pcm帧数据
        while (remain_size > 0)
        {
            size_t real_size = 0;
            
            void * in_frame_start = xRingbufferReceiveUpTo(chatbot_handle.sr2encoder_rb, &real_size, portMAX_DELAY,remain_size);
            memcpy(pcm_data + cur_size, in_frame_start, real_size);
            cur_size += real_size;
            remain_size -= real_size;
            vRingbufferReturnItem(chatbot_handle.sr2encoder_rb, in_frame_start);
        }

        // 开始编码
        esp_audio_err_t ret = esp_opus_enc_process(encoder, &in_frame, &out_frame);
        if (ret != ESP_AUDIO_ERR_OK)
        {
            ESP_LOGE(TAG, "esp_opus_enc_process failed");
            continue;
        }
        
        // 将编码后的数据写入发送环形缓冲区
        xRingbufferSend(chatbot_handle.encoder2ws_rb, out_frame.buffer, out_frame.encoded_bytes, portMAX_DELAY);
    }
}