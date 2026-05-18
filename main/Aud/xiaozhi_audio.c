#include "xiaozhi_audio.h"

#define I2S_MAX_KEEP SOC_I2S_NUM

i2s_chan_handle_t tx_handle;
i2s_chan_handle_t rx_handle;
esp_codec_dev_handle_t codec_dev;

static i2c_master_bus_handle_t i2c_bus_handle;

// 初始化 ES8311 音频设备：建立 I2C/I2S、创建 codec 句柄，并设置采样率、音量和录音增益。
void xiaozhi_audio_Init(void)
{
    xiaozhi_audio_i2c_init();
    xiaozhi_audio_i2s_init();

    audio_codec_i2s_cfg_t i2s_cfg =
        {.rx_handle = rx_handle,
         .tx_handle = tx_handle};

    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .bus_handle = i2c_bus_handle, // 新版本的写法I2C句柄需要设置
    };
    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();

    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = 7,
        .use_mclk = true,
    };
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);

    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,             // es8311_codec_new 获取到的接口实现
        .data_if = data_if,                   // audio_codec_new_i2s_data 获取到的数据接口实现
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT // 设备同时支持录制和播放
    };
    codec_dev = esp_codec_dev_new(&dev_cfg);
    esp_codec_dev_set_out_vol(codec_dev, 30.0);

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
    };
    esp_codec_dev_open(codec_dev, &fs);
    esp_codec_dev_set_in_gain(codec_dev, 15.0);
}

// 初始化 ESP32-S3 到 ES8311 的 I2C 控制总线，用于配置 codec 寄存器。
void xiaozhi_audio_i2c_init(void)
{
    i2c_master_bus_config_t i2c_bus_config = {0};
    i2c_bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_bus_config.i2c_port = I2C_NUM_0;
    i2c_bus_config.scl_io_num = 1;
    i2c_bus_config.sda_io_num = 0;
    // glitch_ignore_cnt = I2C 总线对 SDA/SCL 上短于 N 个 APB 时钟周期的噪声脉冲"视而不见"，防止误触发通信错误。 数值越大越抗干扰，但响应越慢。
    i2c_bus_config.glitch_ignore_cnt = 7;
    // I2C通信引脚一般开漏,硬件没有添加上拉电阻,使用ESP32-S3自带的
    i2c_bus_config.flags.enable_internal_pullup = true;
    i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
}

// 初始化 I2S 收发通道，用于麦克风 PCM 采集和扬声器 PCM 播放。
void xiaozhi_audio_i2s_init(void)
{

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = 3,
            .bclk = 2,
            .ws = 5,
            .dout = 6,
            .din = 4,
        },
    };

    /////////////////////////////////////////////////////////////
    i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);

    // 初始化I2S通信协议
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    // I2S发送使能
    i2s_channel_enable(tx_handle);
    // I2S接受使能
    i2s_channel_enable(rx_handle);
    /////////////////////////////////////////////////////////////
}

// 向 ES8311 写入 PCM 数据，最终从扬声器播放。
void xiaozhi_audio_write(void *data, size_t len)
{
    esp_codec_dev_write(codec_dev, data, len);
}

// 从 ES8311 读取 PCM 数据，供 SR 和 encoder 使用。
void xiaozhi_audio_read(void *data, size_t len)
{
    esp_codec_dev_read(codec_dev, data, len);
}
