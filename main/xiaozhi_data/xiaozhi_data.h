#ifndef __XIAOZHI_DATA_H__
#define __XIAOZHI_DATA_H__
#include "stdbool.h"
#include "esp_afe_sr_models.h"

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

} xiaozhi_data_t;

typedef struct
{
    char *emoji;
    char *text;
} EMOJI;

// [CLAUDE_OPT] extern 声明需要带变量名，这样其他 .c 文件 include 此头文件即可访问该全局变量
extern xiaozhi_data_t xiaozhi_data;
extern EMOJI xiaozhi_emoji[24];

#endif /* __XIAOZHI_DATA_H__ */
