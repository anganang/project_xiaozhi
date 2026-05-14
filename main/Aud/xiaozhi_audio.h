#ifndef __XIAOZHI_AUDIO_H__
#define __XIAOZHI_AUDIO_H__

#include <stddef.h>
#include "driver/i2s_std.h"
#include "driver/i2c_master.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

void xiaozhi_audio_Init(void);

void xiaozhi_audio_i2c_init(void);
void xiaozhi_audio_i2s_init(void);

void xiaozhi_audio_write(void *data, size_t len);
void xiaozhi_audio_read(void *data, size_t len);

#endif /* __XIAOZHI_AUDIO_H__ */
