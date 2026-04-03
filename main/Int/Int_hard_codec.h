#ifndef __INT_HARD_CODEC_H__
#define __INT_HARD_CODEC_H__

#include "driver/i2c_master.h"
#include "driver/i2s_std.h"

#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "esp_log.h"

void Int_Hard_Codec_Init(void);
void Int_Hard_Codec_Play(const uint8_t *data, int data_size);
void Int_Hard_Codec_Record(uint8_t *data, int len);
void Int_Hard_Codec_Open(void);
void Int_Hard_Codec_Close(void);

#endif /* __INT_HARD_CODEC_H__ */
