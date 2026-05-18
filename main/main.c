#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi.h"
#include "xiaozhi_lvgl.h"
#include "xiaozhi_http.h"
#include "xiaozhi_audio.h"
#include "xiaozhi_sr.h"
#include "xiaozhi_data.h"
#include "xiaozhi_encoder.h"
#include "xiaozhi_decoder.h"
#include "xiaozhi_ws.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#define TAG "MAIN"

void xiaozhi_callback(void *button_handle, void *usr_data);
static void create_buffer(void);
static void parse_server_text(char *text, uint32_t len);
static void parse_server_audio(uint8_t *data, size_t len);
static void wakeup_cb(void);
static void vad_cb(void);
static void websocket_send_task(void *arg);

// 程序入口：初始化 UI、音频、WiFi、HTTP、WebSocket、SR、编解码任务，并把各模块缓冲区接起来。
void app_main(void)
{
    xiaozhi_data.event_flag_group = xEventGroupCreate();
    xiaozhi_data.text_cb = parse_server_text;
    xiaozhi_data.audio_cb = parse_server_audio;
    xiaozhi_data.wakeup_cb = wakeup_cb;
    xiaozhi_data.vad_cb = vad_cb;
    xiaozhi_data.server_state = SERVER_STATE_IDLE;

    xiaozhi_button_Init();
    xiaozhi_button2_cb(BUTTON_SINGLE_CLICK, xiaozhi_callback, (void *)1);
    xiaozhi_button2_cb(BUTTON_DOUBLE_CLICK, xiaozhi_callback, (void *)2);
    xiaozhi_button3_cb(BUTTON_LONG_PRESS_HOLD, xiaozhi_callback, (void *)3);

    // 判断wifi是否连接成功

    xiaozhi_lvgl_Init();
    lv_example_get_started_1();
    xiaozhi_audio_Init();
    wifi_init_sta();
    xiaozhi_http_client_init();
    xiaozhi_ws_init();
    create_buffer();
    xiaozhi_sr_init();
    xiaozhi_encoder_init();
    xiaozhi_decoder_init();
    xTaskCreatePinnedToCoreWithCaps(websocket_send_task, "websocket_send", 16 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// 按键事件回调：根据按键编号执行对应动作，目前按钮 1 用于清除 WiFi 配置并重启配网。
void xiaozhi_callback(void *button_handle, void *usr_data)
{
    uint8_t id = (int)usr_data;
    switch (id)
    {
    case 1:
        printf("Button 1 single click event triggered and clear wifi!\n");
        wifi_clear_reset();
        break;
    case 2:
        printf("Button 2 double click event triggered!\n");
        break;
    case 3:
        printf("Button 3 single click event triggered!\n");
        break;
    default:
        break;
    }
}

// 创建音频链路共用的三个 ringbuffer：SR->encoder、encoder->WebSocket、WebSocket->decoder。
static void create_buffer(void)
{
    // SR 输出 PCM，encoder 按一帧需要的长度持续取走。
    xiaozhi_data.sr_encoder_handler = xRingbufferCreateWithCaps(64 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    // encoder 输出完整 OPUS 帧，websocket 按帧发送。
    xiaozhi_data.encoder_websocket_handler = xRingbufferCreateWithCaps(64 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    // websocket 收到完整 OPUS 帧，decoder 按帧解码播放。
    xiaozhi_data.websocket_decoder_handler = xRingbufferCreateWithCaps(64 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
}

// 处理服务器返回的 JSON 文本消息：推进 hello 握手、更新 TTS 文本/状态，并刷新 LLM 表情。
static void parse_server_text(char *text, uint32_t len)
{
    char *json_text = heap_caps_malloc(len + 1, MALLOC_CAP_SPIRAM);
    if (json_text == NULL)
    {
        return;
    }

    memcpy(json_text, text, len);
    json_text[len] = '\0';

    cJSON *root = cJSON_Parse(json_text);
    if (root == NULL)
    {
        ESP_LOGE(TAG, "server text is not json: %s", json_text);
        free(json_text);
        return;
    }

    cJSON *type = cJSON_GetObjectItem(root, "type");
    if (cJSON_IsString(type) && strcmp(type->valuestring, "hello") == 0)
    {
        xEventGroupSetBits(xiaozhi_data.event_flag_group, HELLO_BIT);
    }
    else if (cJSON_IsString(type) && strcmp(type->valuestring, "tts") == 0)
    {
        cJSON *state = cJSON_GetObjectItem(root, "state");
        if (cJSON_IsString(state) && strcmp(state->valuestring, "sentence_start") == 0)
        {
            cJSON *dialog_text = cJSON_GetObjectItem(root, "text");
            if (cJSON_IsString(dialog_text))
            {
                xiaozhi_lvgl_set_dialog(dialog_text->valuestring);
            }
        }
        else if (cJSON_IsString(state) && strcmp(state->valuestring, "start") == 0)
        {
            xiaozhi_data.server_state = SERVER_STATE_SPEECH;
            xiaozhi_lvgl_set_title("说话中....");
        }
        else if (cJSON_IsString(state) && strcmp(state->valuestring, "stop") == 0)
        {
            xiaozhi_data.server_state = SERVER_STATE_IDLE;
            xiaozhi_lvgl_set_title("AI XiaoZhi");
        }
    }
    else if (cJSON_IsString(type) && strcmp(type->valuestring, "llm") == 0)
    {
        cJSON *emotion = cJSON_GetObjectItem(root, "emotion");
        if (cJSON_IsString(emotion))
        {
            xiaozhi_lvgl_set_emoji(emotion->valuestring);
        }
    }

    cJSON_Delete(root);
    free(json_text);
}

// 处理服务器返回的 OPUS 音频数据：按完整帧写入 decoder 使用的 ringbuffer。
static void parse_server_audio(uint8_t *data, size_t len)
{
    if (xiaozhi_data.websocket_decoder_handler == NULL || data == NULL || len == 0)
    {
        return;
    }

    xRingbufferSend(xiaozhi_data.websocket_decoder_handler, data, len, portMAX_DELAY);
}

// SR 检测到唤醒词后的回调：空闲时启动对话，服务器播报时先 abort 再重新进入唤醒对话。
static void wakeup_cb(void)
{
    if (xiaozhi_data.server_state == SERVER_STATE_IDLE)
    {
        xiaozhi_ws_check_wakeup();
    }
    else if (xiaozhi_data.server_state == SERVER_STATE_SPEECH)
    {
        xiaozhi_ws_send_text("{\"type\":\"abort\",\"reason\":\"wake_word_detected\"}");
        xiaozhi_ws_send_text("{\"type\":\"listen\",\"state\":\"detect\",\"text\":\"你好小智\"}");
    }
}

// VAD 状态变化回调：用户开始说话时通知服务器 start，静默时通知服务器 stop。
static void vad_cb(void)
{
    if (xiaozhi_data.vad_current_state == VAD_SPEECH)
    {
        if (xiaozhi_data.server_state == SERVER_STATE_IDLE)
        {
            xiaozhi_ws_send_text("{\"type\":\"listen\",\"state\":\"start\",\"mode\":\"manual\"}");
            xiaozhi_data.server_state = SERVER_STATE_LISTEN;
            xiaozhi_lvgl_set_title("聆听中....");
        }
    }
    else if (xiaozhi_data.vad_current_state == VAD_SILENCE)
    {
        if (xiaozhi_data.server_state != SERVER_STATE_SPEECH)
        {
            xiaozhi_ws_send_text("{\"type\":\"listen\",\"state\":\"stop\"}");
            xiaozhi_data.server_state = SERVER_STATE_IDLE;
            xiaozhi_lvgl_set_title("AI XiaoZhi");
        }
    }
}

// WebSocket 发送任务：持续取 encoder 输出的 OPUS 帧，只在服务器处于 LISTEN 状态时发送。
static void websocket_send_task(void *arg)
{
    while (1)
    {
        size_t len = 0;
        void *opus_data = xRingbufferReceive(xiaozhi_data.encoder_websocket_handler, &len, portMAX_DELAY);
        if (opus_data != NULL)
        {
            if (xiaozhi_data.server_state == SERVER_STATE_LISTEN)
            {
                xiaozhi_ws_send_opus(opus_data, len);
            }
            vRingbufferReturnItem(xiaozhi_data.encoder_websocket_handler, opus_data);
        }
    }
}
