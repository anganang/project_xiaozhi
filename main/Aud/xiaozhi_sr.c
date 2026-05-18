#include "xiaozhi_sr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

#define TAG_RC "xiaozhi Speech Recognition"

static esp_afe_sr_iface_t *afe_handle = NULL;
esp_afe_sr_data_t *afe_data = NULL;

// 测试唤醒回调：当前只打印日志，正式唤醒流程由 main 注册的 wakeup_cb 接管。
void xiaozhi_wakeup_cb(void)
{
    ESP_LOGE(TAG_RC, "xiaozhi has been activated");
}

// VAD 状态变化回调：当前只打印日志，用于观察说话/静默状态切换。
void xiaozhi_status_cb(void)
{
    ESP_LOGE(TAG_RC, "xiaozhi's condition has changed");
}

// AFE 喂数据任务：持续从 ES8311 读取 PCM，并送入 ESP-SR 前端做唤醒词和 VAD 检测。
void feed_Task(void *arg)
{
    // 获取每次数据的采样点
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    // 获取通道数量
    int audio_feed_channel_number = afe_handle->get_feed_channel_num(afe_data);
    // 存储数据
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * audio_feed_channel_number);

    while (1)
    {
        xiaozhi_audio_read(i2s_buff, audio_chunksize * sizeof(int16_t) * audio_feed_channel_number);
        afe_handle->feed(afe_data, i2s_buff);
    }
}

// AFE 检测任务：读取唤醒词/VAD 结果，唤醒后把用户说话的 PCM 推给 encoder。
void detect_Task(void *arg)
{

    while (1)
    {
        // 查看语音检测结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        // 检测是否有唤醒词
        wakenet_state_t wakeup_state = result->wakeup_state;

        if (wakeup_state == WAKENET_DETECTED)
        {
            xiaozhi_data.is_wakeup = true;
            // 唤醒回调赋值完
            if (xiaozhi_data.wakeup_cb != NULL)
            {
                // 还没实现具体逻辑
                xiaozhi_data.wakeup_cb();
            }
        }

        if (xiaozhi_data.is_wakeup)
        {
            xiaozhi_data.vad_current_state = result->vad_state;
            if (xiaozhi_data.vad_current_state != xiaozhi_data.vad_last_state && xiaozhi_data.vad_cb != NULL)
            {
                xiaozhi_data.vad_cb();
            }
            xiaozhi_data.vad_last_state = xiaozhi_data.vad_current_state;
        }

        if (xiaozhi_data.is_wakeup && xiaozhi_data.vad_current_state == VAD_SPEECH)
        {
            // vad_cache 是唤醒前后 AFE 缓存的语音片段，先送给 encoder 防止开头丢字。
            if (result->vad_cache_size > 0)
            {
                xRingbufferSend(xiaozhi_data.sr_encoder_handler, result->vad_cache, result->vad_cache_size, pdMS_TO_TICKS(5000));
            }

            xRingbufferSend(xiaozhi_data.sr_encoder_handler, result->data, result->data_size, pdMS_TO_TICKS(5000));
        }
    }
}

// 初始化 ESP-SR：加载 model 分区里的唤醒模型，创建 AFE，并启动 feed/detect 两个任务。
void xiaozhi_sr_init(void)
{
    // 用es8311获取音频数据

    srmodel_list_t *models = esp_srmodel_init("model");
    if (models == NULL)
    {
        ESP_LOGE(TAG_RC, "speech recognition model partition not found");
        return;
    }

    // 声学前端配置，去噪，唤醒词检测，语音状态识别
    // 通道类型，M检测的数据通过麦克风通道传输
    // 数据模型，小志检测
    // 语音识别场景
    // AFE_MODE_HIGH_PERF功耗大但准确率高
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_config->vad_min_noise_ms = 1000; // 噪声或静默的最小持续时间（以毫秒为单位）。
    afe_config->vad_min_speech_ms = 128; // 语音的最小时长（以毫秒为单位）。
    afe_config->vad_mode = VAD_MODE_3;   // 众数越大，语音触发概率就越高。
    // 当前硬件没有做扬声器回采，先关闭 AEC/SE/NS，减少无效处理带来的误判和内存占用。
    afe_config->aec_init = false;
    afe_config->se_init = false;
    afe_config->ns_init = false;
    afe_config->wakenet_init = true;
    afe_config->wakenet_mode = DET_MODE_90;
    afe_config->afe_ringbuf_size = 4 * 1024;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    // 初始化语音识别框架
    afe_handle = esp_afe_handle_from_config(afe_config);
    //
    afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    xTaskCreatePinnedToCoreWithCaps(feed_Task, "feed", 8 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(detect_Task, "detect", 4 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}
