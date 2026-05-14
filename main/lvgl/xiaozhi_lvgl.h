#ifndef __XIAOZHI_LVGL_H__
#define __XIAOZHI_LVGL_H__

#include "xiaozhi_lcd.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

void xiaozhi_lvgl_Init(void);

void lv_example_get_started_1(void);

void xiaozhi_QR(char *data);

void xiaozhi_delQR(void);

// 6.封装修改标题的方法
void xiaozhi_lvgl_set_title(char *title);
// 7.封装修改emoji的方法
void xiaozhi_lvgl_set_emoji(char *emoji);
// 8修改dialog的方法
void xiaozhi_lvgl_set_dialog(char *dialog);

#endif /* __XIAOZHI_LVGL_H__ */
