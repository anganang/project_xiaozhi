#include "xiaozhi_decoder.h"
#include "esp_err.h"

esp_audio_dec_handle_t opus_decoder;

// 解码任务：从 WebSocket->decoder ringbuffer 取 OPUS 帧，解码成 PCM 后交给 ES8311 播放。
void decoder_task(void *arg)
{
    // 存储需要解码的opus数据
    esp_audio_dec_in_raw_t input_opus_frame = {
        .buffer = NULL,
        .len = 0,
    };
    // 存储解码完成的PCM数据
    esp_audio_dec_out_frame_t output_pcm_frame = {
        .buffer = heap_caps_malloc(8 * 1024, MALLOC_CAP_SPIRAM), // 存储解码完成PCM数据
        .len = 8 * 1024};

    while (1)
    {
        //!!!!!!!将来需要从WEBSCOKET-DECODER缓冲区提取出OPUS数据进行解码
        // 需要从WEBSCOKET_DECODER缓冲区提取出OPUS数据,进行解码
        size_t opus_len = 0;
        void *receive_opus_data = xRingbufferReceive(xiaozhi_data.websocket_decoder_handler, &opus_len, portMAX_DELAY);
        input_opus_frame.buffer = receive_opus_data;
        input_opus_frame.len = opus_len;

        // 持续解码:只要解码的数据还有,持续解码
        while (input_opus_frame.len > 0)
        {
            // 解码逻辑
            esp_audio_dec_info_t aud_info;
            esp_err_t ret = esp_opus_dec_decode(opus_decoder, &input_opus_frame, &output_pcm_frame, &aud_info);
            if (ret != ESP_OK || input_opus_frame.consumed == 0)
            {
                // 解码失败或没有消费输入时必须退出，否则 len 不变会在坏包上死循环。
                break;
            }
            // 解码数据全部解码完成不在解码
            input_opus_frame.len -= input_opus_frame.consumed;    // 表示解码器已经解码完毕的数据长度
            input_opus_frame.buffer += input_opus_frame.consumed; // 表示从这个位置继续解码

            // 测试代码:将解码出来PCM原始数据通过ES8311通过喇叭播放出来
            xiaozhi_audio_write(output_pcm_frame.buffer, output_pcm_frame.decoded_size);
        }

        // 解码完成,告知WEBSCOKET-ENCODER缓冲区,PCM数据已经解码完成,数据释放掉!!!!!
        vRingbufferReturnItem(xiaozhi_data.websocket_decoder_handler, receive_opus_data);
    }
}

// 初始化 OPUS 解码器：配置 16K 单声道 60ms 帧，并启动持续解码播放任务。
void xiaozhi_decoder_init(void)
{

    // 1.opus解码器相关配置
    esp_opus_dec_cfg_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,            // opus音频采样频率
        .channel = ESP_AUDIO_MONO,                           // 单通道
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS, // 数据帧的时长
        .self_delimited = false,                             // opus数据包如果出现:语音，文字，图片..可以拆分处理!!!!!
    };

    // 2.开启解码器
    esp_opus_dec_open(&opus_cfg, sizeof(opus_cfg), &opus_decoder);

    // 3.创建一个任务用于持续解码->opus音频数据,转换为PCM数据!!!!!
    // 创建一个解码的任务
    xTaskCreatePinnedToCoreWithCaps(decoder_task, "decoder_task", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}
