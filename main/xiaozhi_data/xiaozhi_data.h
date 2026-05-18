#ifndef __XIAOZHI_DATA_H__
#define __XIAOZHI_DATA_H__
#include "stdbool.h"
#include "esp_afe_sr_models.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"

// 小智服务器状态：用于区分空闲、正在听用户说话、服务器正在播报，避免音频链路互相打架。
typedef enum
{
    SERVER_STATE_IDLE = 0,
    SERVER_STATE_SPEECH,
    SERVER_STATE_LISTEN,
} server_state_t;

typedef struct
{
    char *websocket_url; // 存储websocket通信协议地址
    char *user_token;    // 存储身份凭证token
    char *code;          // 存储激活码
    bool is_active;      // 判断是否激活

    void (*wakeup_cb)(void);
    bool is_wakeup;

    // 唤醒回调
    vad_state_t vad_current_state; // SR语音检测到当前状态
    vad_state_t vad_last_state;    // SR语音检测到上一状态
    void (*vad_cb)(void);          // SR语音检测模块:检测说话->静默,静默->说话状态发

    // 缓冲区:SR->扔原始数据,编码器从缓冲区去原始数据
    RingbufHandle_t sr_encoder_handler;

    // 缓冲区:编码器ENCODER->WEBSOCKET客户端使用缓冲区
    RingbufHandle_t encoder_websocket_handler;

    // 缓冲区:websocket与解码器使用缓冲区->今天目前没有webscoket客户端模块
    RingbufHandle_t websocket_decoder_handler;

    // 服务器返回文本和音频时由 websocket 模块回调给 main 处理
    void (*text_cb)(char *text, uint32_t len);
    void (*audio_cb)(uint8_t *data, size_t len);

    // websocket 握手流程使用的事件组
    EventGroupHandle_t event_flag_group;

    // 当前服务器交互状态，决定是否发送用户音频、是否允许打断服务器播报。
    server_state_t server_state;

} xiaozhi_data_t;

typedef struct
{
    char *emoji;
    char *text;
} EMOJI;

// [CLAUDE_OPT] extern 声明需要带变量名，这样其他 .c 文件 include 此头文件即可访问该全局变量
extern xiaozhi_data_t xiaozhi_data;
extern EMOJI xiaozhi_emoji[24];

#define CONNECT_BIT (1 << 0)
#define HELLO_BIT (1 << 1)

#endif /* __XIAOZHI_DATA_H__ */
