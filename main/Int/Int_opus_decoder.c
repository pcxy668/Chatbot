#include "Int_opus_decoder.h"

#define TAG "Opus-decoder"

esp_audio_dec_handle_t dec_handle = NULL;

void decoder_task(void *pvParameters);

void Int_Opus_Decoder_Init(void)
{
    // 配置Opus解码器
    esp_opus_dec_cfg_t opus_cfg = {                     
    .sample_rate       = ESP_AUDIO_SAMPLE_RATE_16K, 
    .channel           = ESP_AUDIO_MONO,
    .frame_duration    = ESP_OPUS_DEC_FRAME_DURATION_60_MS, 
    .self_delimited    = false
    };

    esp_opus_dec_open((void *)&opus_cfg, sizeof(opus_cfg), &dec_handle);

    xTaskCreatePinnedToCoreWithCaps(decoder_task, "decoder_task", 16 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
}

void decoder_task(void *pvParameters)
{
    uint8_t *out_data = heap_caps_malloc(4 * 1024, MALLOC_CAP_SPIRAM);

    // 配置输入输出帧
    esp_audio_dec_in_raw_t in_frame = {
        .buffer = NULL,
        .len = 0,
        .consumed = 0,
    };
    esp_audio_dec_out_frame_t out_frame = {
        .buffer = out_data,
        .len = 4 * 1024, 
        .needed_size = 0,
    };
    
    esp_audio_dec_info_t info = {0};

    while (1)
    {
        // 获取opus数据
        in_frame.buffer = (uint8_t *)xRingbufferReceive(chatbot_handle.ws2decoder_rb,(size_t *)&(in_frame.len), portMAX_DELAY);

        // 开始解码
        esp_audio_err_t ret = esp_opus_dec_decode(dec_handle, &in_frame, &out_frame, &info);
        if (ret != ESP_AUDIO_ERR_OK)
        {
            ESP_LOGE(TAG, "esp_opus_dec_process failed");
            continue;
        }

        vRingbufferReturnItem(chatbot_handle.ws2decoder_rb, in_frame.buffer);

        // 播放解码后的数据
        Int_Hard_Codec_Play(out_frame.buffer, out_frame.decoded_size);
    }
}