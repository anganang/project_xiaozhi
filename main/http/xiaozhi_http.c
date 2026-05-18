#include "xiaozhi_http.h"

// [FIXED] 取消注释，HTTP模块使用自己的TAG，不再依赖wifi头文件中的定义
#define TAG "XIAOZHI_HTTP"
#define MAX_HTTP_OUTPUT_BUFFER 2048
#define MAX_HTTP_RECV_BUFFER 512

char uuid[37];
uint8_t eth_mac[6];
char mac_ptr[50];
uint32_t recv_len = 0; // 当前已接收的响应体字节数

esp_http_client_handle_t client;

static void xiaozhi_http_set_client(void);

void xiaozhi_http_json(const char *json_str);

/**
 * 生成 UUID v4 字符串
 * @param uuid_str 输出缓冲区，至少 37 字节（36字符 + '\0'）
 */
static void uuid_v4_generate(char *uuid_str)
{
    uint8_t uuid[16];

    // 用硬件随机数填充 16 字节
    esp_fill_random(uuid, sizeof(uuid));

    // 设置版本号: version 4 (0100xxxx)
    uuid[6] = (uuid[6] & 0x0F) | 0x40;

    // 设置变体: variant 1 (10xxxxxx)
    uuid[8] = (uuid[8] & 0x3F) | 0x80;

    // 格式化为标准 UUID 字符串: 8-4-4-4-12
    snprintf(uuid_str, 37,
             "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             uuid[0], uuid[1], uuid[2], uuid[3],
             uuid[4], uuid[5],
             uuid[6], uuid[7],
             uuid[8], uuid[9],
             uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

/**
 * HTTP 事件回调函数
 *
 * 事件触发顺序（一次完整的 HTTP 请求）:
 *   1. HTTP_EVENT_ON_CONNECTED  — TCP/TLS 连接建立
 *   2. HTTP_EVENT_HEADER_SENT   — 请求头已发送到服务器
 *   3. HTTP_EVENT_ON_HEADER     — 收到响应头（每个 header 触发一次）
 *   4. HTTP_EVENT_ON_DATA       — 收到响应体数据（可能触发多次，分块到达）
 *   5. HTTP_EVENT_ON_FINISH     — 响应接收完毕
 *   6. HTTP_EVENT_DISCONNECTED  — 连接断开（正常关闭或异常断开）
 *
 * 特殊事件:
 *   - HTTP_EVENT_ERROR    — 发生错误
 *   - HTTP_EVENT_REDIRECT — 服务器返回 3xx 重定向
 *
 * 数据流: ON_HEADER 中根据 Content-Length 分配 buffer → ON_DATA 中逐块拷贝 → ON_FINISH 中处理完整响应并释放
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    // [CLAUDE_OPT] static 变量在多次请求间保持状态，必须在 FINISH/DISCONNECTED 中正确重置
    static char *output_buffer; // 用于存储事件处理程序中 HTTP 请求响应的缓冲区
    static int output_len;      // 记录读取的字节数（暂未使用，保留兼容）

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        // 请求过程中发生错误（DNS 解析失败、连接超时等）
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        // TCP/TLS 握手完成，连接已建立，即将发送请求
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        // 请求头（包括 method、path、headers）已全部发送到服务器
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        // 收到一个响应头字段，每个 header 触发一次此事件
        // evt->header_key = 头字段名, evt->header_value = 头字段值
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        // 只在收到 Content-Length 头时分配接收缓冲区
        if (strcasecmp(evt->header_key, "Content-Length") == 0)
        {
            // [CLAUDE_OPT] +1 为末尾 '\0' 预留空间，使用 SPIRAM 避免占用内部 RAM
            int content_length = atoi(evt->header_value);
            output_buffer = heap_caps_malloc(content_length + 1, MALLOC_CAP_SPIRAM);
            if (output_buffer == NULL)
            {
                ESP_LOGE(TAG, "failed to allocate memory for output buffer");
                return ESP_FAIL;
            }
            // [CLAUDE_OPT] 预置末尾 '\0'，确保即使数据不完整也能安全当字符串使用
            output_buffer[content_length] = '\0';
        }
        break;
    case HTTP_EVENT_ON_DATA:
        // 收到一块响应体数据，大响应会分多次触发此事件
        // evt->data = 本次数据指针, evt->data_len = 本次数据长度
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        // [CLAUDE_OPT] 防御: 如果 Content-Length 头缺失导致 buffer 未分配，跳过拷贝避免崩溃
        if (output_buffer == NULL)
        {
            ESP_LOGE(TAG, "output_buffer is NULL, skipping data copy");
            break;
        }
        memcpy(output_buffer + recv_len, evt->data, evt->data_len);
        recv_len += evt->data_len;
        break;
    case HTTP_EVENT_ON_FINISH:
        // 响应接收完毕（所有数据已通过 ON_DATA 事件到达）
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        if (output_buffer != NULL)
        {
            output_buffer[recv_len] = '\0';
            ESP_LOGI(TAG, "response: %s", output_buffer);
            // [CLAUDE_OPT] 解析 JSON 响应并更新 UI，这是显示内容的关键调用
            xiaozhi_http_json(output_buffer);
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        recv_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        // 连接断开（可能是正常关闭，也可能是网络异常）
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        // [CLAUDE_OPT] 异常断开时也要清理，防止内存泄漏
        if (output_buffer != NULL)
        {
            free(output_buffer);
            output_buffer = NULL;
        }
        output_len = 0;
        // [CLAUDE_OPT] 重置 recv_len
        recv_len = 0;
        break;
    case HTTP_EVENT_REDIRECT:
        // 服务器返回 3xx 重定向，需要设置新的 header 并跟随重定向
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        esp_http_client_set_header(evt->client, "From", "user@example.com");
        esp_http_client_set_header(evt->client, "Accept", "text/html");
        esp_http_client_set_redirection(evt->client);
        break;
    }
    return ESP_OK;
}

// 初始化 HTTP OTA 客户端：设置服务器地址、请求头并发起一次 OTA/激活信息请求。
void xiaozhi_http_client_init(void)
{
    esp_http_client_config_t config =
        {
            // 请求的域名
            .host = "api.tenclass.net",
            // 请求的路径
            .path = "/xiaozhi/ota/",
            // 加密传输。另一种TCP是明文传输
            .transport_type = HTTP_TRANSPORT_OVER_SSL,
            // 事件回调
            .event_handler = _http_event_handler,
            // 证书，证书需要放到外部内存
            .crt_bundle_attach = esp_crt_bundle_attach,
            // 请求方法
            .method = HTTP_METHOD_POST

        };

    client = esp_http_client_init(&config);

    xiaozhi_http_set_client();

    // 向服务器发起HTTP请求
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP GET OK");
    }
    else
    {
        ESP_LOGE(TAG, "HTTP GET FAIL");
    }
}

// 设置小智 OTA 接口需要的请求头，包括语言、User-Agent、Client-Id 和 Device-Id。
static void xiaozhi_http_set_client(void)
{
    esp_http_client_set_header(client, "Accept-Language", "zh-CN");
    esp_http_client_set_header(client, "User-Agent", "xingzhi-cube-1.54tft-wifi/1.0.1");

    // 客户端唯一标识符
    uuid_v4_generate(uuid);
    ESP_LOGI(TAG, "UUID: %s", uuid);
    esp_http_client_set_header(client, "Client-Id", uuid);

    // 获取mac地址
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    // 将mac地址转化为字符串
    sprintf(mac_ptr, "%02X:%02X:%02X:%02X:%02X:%02X",
            eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);

    esp_http_client_set_header(client, "Device-Id", mac_ptr);
}

// 解析 OTA 返回的 JSON：保存 WebSocket 地址/token，更新激活状态和屏幕提示。
void xiaozhi_http_json(const char *json_str)
{
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "JSON parse failed");
        return;
    }

    cJSON *websocket = cJSON_GetObjectItem(root, "websocket");
    if (cJSON_IsObject(websocket))
    {
        cJSON *url_item = cJSON_GetObjectItem(websocket, "url");
        cJSON *token_item = cJSON_GetObjectItem(websocket, "token");
        // [CLAUDE_OPT] 必须 strdup 拷贝，因为 cJSON_Delete 后原指针失效
        if (url_item != NULL && url_item->valuestring != NULL)
        {
            free(xiaozhi_data.websocket_url);
            xiaozhi_data.websocket_url = strdup(url_item->valuestring);
        }
        if (token_item != NULL && token_item->valuestring != NULL)
        {
            free(xiaozhi_data.user_token);
            xiaozhi_data.user_token = strdup(token_item->valuestring);
        }
        ESP_LOGI(TAG, "websocket url: %s", xiaozhi_data.websocket_url);
    }
    else
    {
        cJSON *mqtt = cJSON_GetObjectItem(root, "mqtt");
        if (cJSON_IsObject(mqtt))
        {
            cJSON *endpoint_item = cJSON_GetObjectItem(mqtt, "endpoint");
            if (cJSON_IsString(endpoint_item))
            {
                ESP_LOGI(TAG, "mqtt endpoint: %s", endpoint_item->valuestring);
            }
        }
        else
        {
            ESP_LOGW(TAG, "websocket and mqtt fields not found");
        }
    }

    cJSON *activation = cJSON_GetObjectItem(root, "activation");
    if (activation != NULL)
    {
        cJSON *code_item = cJSON_GetObjectItem(activation, "code");
        if (code_item != NULL && code_item->valuestring != NULL)
        {
            // [CLAUDE_OPT] strdup 拷贝激活码
            free(xiaozhi_data.code);
            xiaozhi_data.code = strdup(code_item->valuestring);
        }
        xiaozhi_data.is_active = false;

        xiaozhi_lvgl_set_title("PLASE GO TO ACTIVE");
        xiaozhi_lvgl_set_emoji("pain");
        char dialog_info[32];
        snprintf(dialog_info, sizeof(dialog_info), "code:%s", xiaozhi_data.code);
        xiaozhi_lvgl_set_dialog(dialog_info);

        ESP_LOGI(TAG, "code is %s", xiaozhi_data.code);
    }
    else
    {
        xiaozhi_data.is_active = true;
        ESP_LOGI(TAG, "device already activated");

        xiaozhi_lvgl_set_title("AI XiaoZhi");
        xiaozhi_lvgl_set_emoji("happy");
        xiaozhi_lvgl_set_dialog("你好,我是小智");
    }

    cJSON_Delete(root);
}
