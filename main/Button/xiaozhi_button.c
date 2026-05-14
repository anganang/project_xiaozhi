#include "xiaozhi_button.h"
#define TAG "xiaozhi_button"

button_handle_t adc_btn2 = NULL;
button_handle_t adc_btn3 = NULL;

void xiaozhi_button_Init(void)
{
    const button_config_t btn2_cfg = {0};
    button_adc_config_t btn2_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = 7,
        .button_index = 2,
        .min = 0,
        .max = 400,
    };
    esp_err_t ret2 = iot_button_new_adc_device(&btn2_cfg, &btn2_adc_cfg, &adc_btn2);
    if (ret2 != ESP_OK)
    {
        ESP_LOGE("TAG", "Button2 create OK");
    }

    const button_config_t btn3_cfg = {0};
    button_adc_config_t btn3_adc_cfg = {
        .unit_id = ADC_UNIT_1,
        .adc_channel = 7,
        .button_index = 3,
        .min = 1200,
        .max = 1800,
    };
    esp_err_t ret3 = iot_button_new_adc_device(&btn3_cfg, &btn3_adc_cfg, &adc_btn3);
    if (ret3 != ESP_OK)
    {
        ESP_LOGE("TAG", "Button3 create OK");
    }
}

void xiaozhi_button2_cb(button_event_t event, button_cb_t cb, void *user_data)
{
    iot_button_register_cb(adc_btn2, event, NULL, cb, user_data);
}

void xiaozhi_button3_cb(button_event_t event, button_cb_t cb, void *user_data)
{
    iot_button_register_cb(adc_btn3, event, NULL, cb, user_data);
}
