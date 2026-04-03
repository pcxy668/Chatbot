#include "Int_activate.h"

chatbot_handle_t chatbot_handle;
esp_http_client_handle_t client = NULL;

static const char *TAG = "HTTP_CLIENT";
static int body_len = 0;
static char *body = NULL;
static int current_len = 0;
static char *str_cjson = NULL;

static void Int_Activate_ParseResponse(char *str, int len)
{
    cJSON *root = cJSON_ParseWithLength(str,len);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "root is NULL");
        return;
    }

    cJSON *websocket = cJSON_GetObjectItem(root, "websocket");
    if (websocket == NULL)
    {
        ESP_LOGE(TAG, "websocket is NULL");
        return;
    }

    cJSON *url = cJSON_GetObjectItem(websocket, "url");
    if (url == NULL)
    {
        ESP_LOGE(TAG, "url is NULL");
        return;
    }
    char *tmp = url->valuestring;
    chatbot_handle.ws_url = malloc(strlen(tmp) + 1);
    strcpy(chatbot_handle.ws_url, tmp);

    cJSON *token = cJSON_GetObjectItem(websocket, "token");
    if (token == NULL)
    {
        ESP_LOGE(TAG, "token is NULL");
        return;
    }
    tmp = token->valuestring;
    chatbot_handle.ws_token = malloc(strlen(tmp) + 1);
    strcpy(chatbot_handle.ws_token, tmp);

    cJSON *activation = cJSON_GetObjectItem(root, "activation");
    if (activation == NULL)
    {
        chatbot_handle.act_flag = true;
        ESP_LOGI(TAG, "Activated");

        // 初始化Chatbot环形缓冲区
        chatbot_handle.sr2encoder_rb = xRingbufferCreateWithCaps(8 * 1024,RINGBUF_TYPE_BYTEBUF,  MALLOC_CAP_SPIRAM);
        chatbot_handle.encoder2ws_rb = xRingbufferCreateWithCaps(8 * 1024,RINGBUF_TYPE_NOSPLIT,  MALLOC_CAP_SPIRAM);
        chatbot_handle.ws2decoder_rb = xRingbufferCreateWithCaps(8 * 1024,RINGBUF_TYPE_NOSPLIT,  MALLOC_CAP_SPIRAM);
        
        return;
    }
    else
    {
        cJSON *code = cJSON_GetObjectItem(activation, "code");
        tmp = code->valuestring;
        chatbot_handle.act_code = malloc(strlen(tmp) + 1);
        strcpy(chatbot_handle.act_code, tmp);
        chatbot_handle.act_flag = false;
        char text[80] = {0};
        sprintf(text,"请在控制台激活Chatbot\n激活码:%s\n激活后请按2键重启",chatbot_handle.act_code);
        Int_LVGL_ShowText(text);
        Int_LVGL_ShowTip("请激活……");
    }

    cJSON_Delete(root);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            // 根据响应头得到内容长度
            if (strcmp(evt->header_key, "Content-Length") == 0)
            {
                body_len = atoi(evt->header_value);
                body = malloc(body_len + 1);
                body[body_len] = '\0';
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (current_len + evt->data_len <= body_len)
            {
                memcpy(body + current_len, evt->data, evt->data_len);
                current_len += evt->data_len;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            if (body != NULL)
            {
                printf("%s\n", body);
                Int_Activate_ParseResponse(body, body_len);
                free(body);
                body = NULL;
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

static void Int_Activate_Request_SetHeader(void)
{
    esp_http_client_set_header(client, "Host", "api.tenclass.net");
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "User-Agent", "bread-compact-wifi-128x64/1.0.1");

    uint8_t mac[6] = {0};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    sprintf(chatbot_handle.mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    printf("mac_str: %s\n", chatbot_handle.mac_str);
    esp_http_client_set_header(client, "Device-Id", chatbot_handle.mac_str);

    nvs_handle_t my_handle;
    nvs_open("storage", NVS_READWRITE, &my_handle);
    size_t required_size = sizeof(chatbot_handle.uuid);
    esp_err_t ret = nvs_get_str(my_handle, "uuid_chatbot", chatbot_handle.uuid, &required_size);
    if (ret != ESP_OK || (strcmp(chatbot_handle.uuid, "f1643c23-1082-4272-b121-053e5e719255") != 0))
    {
        nvs_set_str(my_handle, "uuid_chatbot", "f1643c23-1082-4272-b121-053e5e719255");
        nvs_commit(my_handle);
        esp_http_client_set_header(client, "Client-Id", "f1643c23-1082-4272-b121-053e5e719255");
    }
    else 
    {
        esp_http_client_set_header(client, "Client-Id", chatbot_handle.uuid);
    }
    nvs_close(my_handle);
}

static void Int_Activate_Request_SetBody(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *application = cJSON_CreateObject();
    cJSON *board = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "application", application);
    cJSON_AddItemToObject(root, "board", board);

    cJSON_AddStringToObject(application, "version", "1.0.1");
    cJSON_AddStringToObject(application, "elf_sha256", esp_app_get_elf_sha256_str());

    cJSON_AddStringToObject(board, "type", "bread-compact-wifi");
    cJSON_AddStringToObject(board, "name", "bread-compact-wifi-128x64");
    cJSON_AddStringToObject(board, "ssid", "chatbox");
    cJSON_AddNumberToObject(board, "rssi", -55);
    cJSON_AddNumberToObject(board, "channel", 1);
    cJSON_AddStringToObject(board, "ip", "192.168.0.1");
    
    str_cjson = cJSON_PrintUnformatted(root);
    esp_http_client_set_post_field(client, str_cjson, strlen(str_cjson));

    cJSON_Delete(root);
}

void Int_Activate_Request_With_Https(void)
{
    esp_http_client_config_t config = {
        .host = "api.tenclass.net",
        .path = "/xiaozhi/ota/",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = HTTP_METHOD_POST,
    };
    ESP_LOGI(TAG, "HTTPS request with hostname and path =>");
    client = esp_http_client_init(&config);

    Int_Activate_Request_SetHeader();
    Int_Activate_Request_SetBody();

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }

    cJSON_free(str_cjson);
    esp_http_client_cleanup(client);    

    // 打印chatbot_handle_t结构体信息
    // printf("ws_url=%s, ws_token=%s, act_code=%s, act_flag=%d\n",
    //     chatbot_handle.ws_url ? chatbot_handle.ws_url : "(null)",
    //     chatbot_handle.ws_token ? chatbot_handle.ws_token : "(null)",
    //     chatbot_handle.act_code ? chatbot_handle.act_code : "(null)",
    //     chatbot_handle.act_flag);
}
