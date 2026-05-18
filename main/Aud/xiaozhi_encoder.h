#ifndef __XIAOZHI_ENCODER_H__
#define __XIAOZHI_ENCODER_H__

#include "esp_opus_enc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "string.h"
#include "xiaozhi_data.h"

void xiaozhi_encoder_init(void);

#endif /* __XIAOZHI_ENCODER_H__ */
