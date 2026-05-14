#ifndef __XIAOZHI_BUTTON_H__
#define __XIAOZHI_BUTTON_H__

#include "button_adc.h"
#include "button_matrix.h"
#include "esp_log.h"
#include "iot_button.h"

void xiaozhi_button_Init(void);

void xiaozhi_button2_cb(button_event_t event, button_cb_t cb, void *user_data);
void xiaozhi_button3_cb(button_event_t event, button_cb_t cb, void *user_data);

#endif /* __XIAOZHI_BUTTON_H__ */
