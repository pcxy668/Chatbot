#ifndef __COM_TYPES_H__
#define __COM_TYPES_H__

#include <stdbool.h>
#include "freertos/ringbuf.h"

typedef enum
{
    CHATBOT_IDLE = 0,
    CHATBOT_SPEAKING,
    CHATBOT_LISTENING
} chatbot_state_t;

typedef struct 
{
    char *ws_url;
    char *ws_token;
    char *act_code;
    bool act_flag;

    bool last_vad_state;
    bool cur_vad_state;
    bool wakeup_state;

    RingbufHandle_t sr2encoder_rb;
    RingbufHandle_t encoder2ws_rb;
    RingbufHandle_t ws2decoder_rb;

    chatbot_state_t state;

    void (*wake_cb)(void);
    void (*vad_changed_cb)(void);

    char mac_str[18];
    char uuid[37];
} chatbot_handle_t;

typedef struct 
{
    char *name;
    char *emoji;
} chatbot_emoji_t;

#endif /* __COM_TYPES_H__ */
