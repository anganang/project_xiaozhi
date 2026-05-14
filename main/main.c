#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "xiaozhi_button.h"
#include "xiaozhi_wifi.h"
#include "xiaozhi_lvgl.h"
#include "xiaozhi_http.h"
#include "xiaozhi_audio.h"
#include "xiaozhi_sr.h"

void xiaozhi_callback(void *button_handle, void *usr_data);

void app_main(void)
{
    xiaozhi_button_Init();
    xiaozhi_button2_cb(BUTTON_SINGLE_CLICK, xiaozhi_callback, (void *)1);
    xiaozhi_button2_cb(BUTTON_DOUBLE_CLICK, xiaozhi_callback, (void *)2);
    xiaozhi_button3_cb(BUTTON_LONG_PRESS_HOLD, xiaozhi_callback, (void *)3);

    // 判断wifi是否连接成功

    xiaozhi_lvgl_Init();
    lv_example_get_started_1();
    xiaozhi_audio_Init();
    xiaozhi_sr_init();
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // wifi_init_sta();
    // xiaozhi_http_client_init();
}

void xiaozhi_callback(void *button_handle, void *usr_data)
{
    uint8_t id = (int)usr_data;
    switch (id)
    {
    case 1:
        printf("Button 1 single click event triggered and clear wifi!\n");
        wifi_clear_reset();
        break;
    case 2:
        printf("Button 2 double click event triggered!\n");
        break;
    case 3:
        printf("Button 3 single click event triggered!\n");
        break;
    default:
        break;
    }
}
