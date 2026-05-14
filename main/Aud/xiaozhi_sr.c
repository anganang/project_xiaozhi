#include "xiaozhi_sr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"

#define TAG_RC "xiaozhi Speech Recognition"

static esp_afe_sr_iface_t *afe_handle = NULL;
esp_afe_sr_data_t *afe_data = NULL;

// 测试小智是否被激活回调
void xiaozhi_wakeup_cb(void)
{
    ESP_LOGE(TAG_RC, "xiaozhi has been activated");
}

// 测试小智的语音状态是否发生变化
void xiaozhi_status_cb(void)
{
    ESP_LOGE(TAG_RC, "xiaozhi's condition has changed");
}

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

// 检测任务
void detect_Task(void *arg)
{

    while (1)
    {
        // 查看语音检测结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        // 检测是否有唤醒词
        wakenet_state_t wakeup_state = result->wakeup_state;
        vad_state_t vad_state = result->vad_state;

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
    }
}

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

    // 初始化语音识别框架
    afe_handle = esp_afe_handle_from_config(afe_config);
    //
    afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

    // 测试代码，观察小智是否被唤醒
    xiaozhi_data.wakeup_cb = xiaozhi_wakeup_cb;
    xiaozhi_data.vad_cb = xiaozhi_status_cb;

    xTaskCreatePinnedToCoreWithCaps(feed_Task, "feed", 8 * 1024, NULL, 5, NULL, 0, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(detect_Task, "detect", 4 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}
