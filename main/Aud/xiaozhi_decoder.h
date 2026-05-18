#ifndef __XIAOZHI_DECODER_H__
#define __XIAOZHI_DECODER_H__

#include "esp_opus_dec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xiaozhi_data.h"
#include "freertos/ringbuf.h"
#include "xiaozhi_audio.h"

// 1.解码器初始化方法
void xiaozhi_decoder_init(void);

#endif /* __XIAOZHI_DECODER_H__ */
