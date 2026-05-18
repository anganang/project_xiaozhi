#include "xiaozhi_encoder.h"
#define TAG "XIAOZHI_ENCODER"

esp_audio_enc_handle_t encoder = NULL;
// PCM数据帧大小:1920     输出opus音频数据:340
int input_pcm_size = 0, out_opus_size = 0;
uint8_t *pcm_data = NULL, *opus_data = NULL;

// 编码任务：从 SR->encoder ringbuffer 拼够一帧 PCM，编码成 OPUS 后写入 encoder->WebSocket ringbuffer。
void encoder_task(void *arg)
{

    // 输入进来原始数据:PCM相关参数配置
    esp_audio_enc_in_frame_t input_pcm_frame = {
        .buffer = pcm_data,
        .len = input_pcm_size, // 需要1920PCM数据,存储到buffer缓冲区里面进行编码
    };
    // 输出OPUS音频数据:OPUS相关参数配置
    esp_audio_enc_out_frame_t output_opus_frame = {
        .buffer = opus_data,
        .len = out_opus_size, // 期望编码 340 OPUS音频数据,存储到buffer缓冲区里面进行输出
    };

    while (1)
    {
        // 原始音频数据,目前放在SR-ENCODER编码器缓冲区当中,从缓冲区提取-1920PCM字节数据。生成340字节OPUS音频数据

        // 第一个参数:缓冲区->SR-ENCODER缓冲区
        // 第二个参数:缓冲区提取数据长度
        // 第三个参数:等待超时的时间
        // 第四个参数:每一次提取出来希望数值
        // 返回数值:这一次提取出来PCM数据
        uint32_t hope_size = input_pcm_size;
        // 将提取出来的PCM数据,内存存储主,交给编码器进行编码
        uint8_t *receive_buffer = input_pcm_frame.buffer;
        while (hope_size > 0)
        {
            // 用于保存每一次提出出来数据实际长度
            size_t received_size = 0;
            void *receive_pcm_data = xRingbufferReceiveUpTo(xiaozhi_data.sr_encoder_handler, &received_size, portMAX_DELAY, hope_size);
            // 下一次期望提取数据,减去当前这一次已经提出取来数据量
            hope_size -= received_size;
            // // 拷贝PCM数据到缓冲区
            memcpy(receive_buffer, receive_pcm_data, received_size);
            receive_buffer += received_size;
            // //每一次缓冲区提取,提取完释放缓冲内部已经提取完空间内存,否则新的数据写入不进来
            vRingbufferReturnItem(xiaozhi_data.sr_encoder_handler, receive_pcm_data);
        }
        // 进行encoder编码:PCM数据转换为OPUS压缩音频数据
        esp_opus_enc_process(encoder, &input_pcm_frame, &output_opus_frame);

        //!!!!!!!!!已经将1920PCM音频原始数据,已经编码成功变为340OPUS音频数据(压缩)
        // output_opus_frame.encoded_bytes:表示编码器,将PCM转换为OPUS音频数据实际长度!
        xRingbufferSend(xiaozhi_data.encoder_websocket_handler, output_opus_frame.buffer, output_opus_frame.encoded_bytes, portMAX_DELAY);
    }
}

// 初始化 OPUS 编码器：配置 16K 单声道 60ms 帧，分配输入/输出缓冲区并启动编码任务。
void xiaozhi_encoder_init(void)
{
    // 1，opus音频格式编码器
    // enable_fec丢包备份
    // enble_dtx省流量
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,            // 编码器采样频率
        .channel = ESP_AUDIO_MONO,                           // 单声道
        .bits_per_sample = ESP_AUDIO_BIT16,                  // 位深
        .bitrate = 32000,                                    // 比特率
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS, // 帧时长
        .application_mode = ESP_OPUS_ENC_APPLICATION_AUDIO,  // 应用模式
        .complexity = 5,                                     // 编码复杂度
        .enable_dtx = false,
        .enable_fec = false,
        .enable_vbr = false // 可变比特率
    };
    // 开启opus编码器
    esp_opus_enc_open(&opus_cfg, sizeof(esp_opus_enc_config_t), &encoder);
    // 编码一帧音频，需要准备多少字节的PCM数据
    esp_opus_enc_get_frame_size(encoder, &input_pcm_size, &out_opus_size);
    ESP_LOGE(TAG, "input_pcm_size:%d, out_opus_size:%d", input_pcm_size, out_opus_size);
    // 准备内存:两个指针,开辟相应内存空间,存储音频数据
    pcm_data = malloc(input_pcm_size);
    opus_data = malloc(out_opus_size);

    // 创建freeRTOS任务,用来轮询进行编码->编码这个活不是一次性
    xTaskCreatePinnedToCoreWithCaps(encoder_task, "encoder_task", 32 * 1024, NULL, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}
