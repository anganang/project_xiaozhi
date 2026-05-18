#include "xiaozhi_ws.h"

#define TAG "XIAOZHI_WS"

extern char uuid[37];
extern char mac_ptr[50];

static esp_websocket_client_handle_t websocket_client = NULL;

// 将字符串转换为小写，主要用于把 MAC 地址格式调整成服务器要求的 Device-Id。
static void to_lowercase(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}

// WebSocket 事件回调：处理连接状态、文本消息和二进制音频消息，并转交给 main 注册的回调。
static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "websocket connected");
        xEventGroupSetBits(xiaozhi_data.event_flag_group, CONNECT_BIT);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        xiaozhi_data.server_state = SERVER_STATE_IDLE;
        xiaozhi_data.is_wakeup = false;
        ESP_LOGI(TAG, "websocket disconnected");
        break;
    case WEBSOCKET_EVENT_DATA:
        // opcode 1 是 JSON 文本，opcode 2 是服务器返回的 OPUS 音频帧。
        if (data->op_code == 1 && xiaozhi_data.text_cb != NULL)
        {
            xiaozhi_data.text_cb((char *)data->data_ptr, data->data_len);
        }
        else if (data->op_code == 2 && xiaozhi_data.audio_cb != NULL)
        {
            xiaozhi_data.audio_cb((uint8_t *)data->data_ptr, data->data_len);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "websocket error");
        break;
    default:
        break;
    }
}

// 初始化 WebSocket 客户端：设置服务器地址、证书和小智协议需要的请求头。
void xiaozhi_ws_init(void)
{
    if (xiaozhi_data.websocket_url == NULL || xiaozhi_data.user_token == NULL)
    {
        ESP_LOGE(TAG, "websocket url or token is NULL");
        return;
    }

    esp_websocket_client_config_t websocket_cfg = {
        .uri = xiaozhi_data.websocket_url,
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .reconnect_timeout_ms = 5000,
        .network_timeout_ms = 5000,
    };

    websocket_client = esp_websocket_client_init(&websocket_cfg);
    if (websocket_client == NULL)
    {
        ESP_LOGE(TAG, "websocket client init failed");
        return;
    }

    esp_websocket_register_events(websocket_client, WEBSOCKET_EVENT_ANY, websocket_event_handler, websocket_client);
    esp_websocket_client_append_header(websocket_client, "Protocol-Version", "1");
    esp_websocket_client_append_header(websocket_client, "Client-Id", uuid);

    to_lowercase(mac_ptr);
    esp_websocket_client_append_header(websocket_client, "Device-Id", mac_ptr);

    size_t auth_len = strlen("Bearer ") + strlen(xiaozhi_data.user_token) + 1;
    char *authorization = heap_caps_malloc(auth_len, MALLOC_CAP_SPIRAM);
    if (authorization == NULL)
    {
        ESP_LOGE(TAG, "authorization header alloc failed");
        return;
    }

    snprintf(authorization, auth_len, "Bearer %s", xiaozhi_data.user_token);
    esp_websocket_client_append_header(websocket_client, "Authorization", authorization);
    free(authorization);
}

// 通过 WebSocket 发送 JSON 文本消息，用于 hello、listen 等控制消息。
void xiaozhi_ws_send_text(char *text)
{
    if (websocket_client == NULL || text == NULL)
    {
        return;
    }

    esp_websocket_client_send_text(websocket_client, text, strlen(text), pdMS_TO_TICKS(5000));
}

// 通过 WebSocket 发送 OPUS 二进制音频帧，供服务器做 ASR/LLM/TTS 处理。
void xiaozhi_ws_send_opus(uint8_t *data, size_t len)
{
    if (websocket_client == NULL || data == NULL || len == 0)
    {
        return;
    }

    esp_websocket_client_send_bin(websocket_client, (const char *)data, len, pdMS_TO_TICKS(5000));
}

// 唤醒后执行 WebSocket 对话准备流程：连接服务器、发送 hello，收到 hello 应答后发送 listen。
void xiaozhi_ws_check_wakeup(void)
{
    if (websocket_client == NULL)
    {
        ESP_LOGE(TAG, "websocket client is NULL");
        return;
    }

    if (!esp_websocket_client_is_connected(websocket_client))
    {
        esp_websocket_client_start(websocket_client);
    }

    EventBits_t connect_result = xEventGroupWaitBits(xiaozhi_data.event_flag_group, CONNECT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if (connect_result & CONNECT_BIT)
    {
        xiaozhi_ws_send_text("{\"type\":\"hello\",\"version\":1,\"transport\":\"websocket\",\"features\":{\"mcp\":true},\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}");
    }

    EventBits_t hello_result = xEventGroupWaitBits(xiaozhi_data.event_flag_group, HELLO_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if (hello_result & HELLO_BIT)
    {
        xiaozhi_ws_send_text("{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"你好小智\"}");
    }
}
