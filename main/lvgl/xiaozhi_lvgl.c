#include "xiaozhi_lvgl.h"

static lv_display_t *lvgl_disp = NULL;

lv_obj_t *qr = NULL;

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t panel_handle;

// 标题节点
lv_obj_t *title = NULL;
// emoji节点
lv_obj_t *emoji = NULL;
// 对话节点
lv_obj_t *dialog = NULL;

static void app_lvgl_init(void)
{
    /* Initialize LVGL */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,       /* LVGL task priority */
        .task_stack = 4096 * 2,   /* LVGL task stack size */
        .task_affinity = -1,      /* LVGL task pinned to core (-1 is no affinity) */
        .task_max_sleep_ms = 500, /* Maximum sleep in LVGL task */
        .timer_period_ms = 5      /* LVGL timer tick period in ms */
    };
    lvgl_port_init(&lvgl_cfg);

    /* Add LCD screen */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = EXAMPLE_LCD_H_RES * PARALLEL_LINES,
        .double_buffer = 1,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = false,

        .color_format = LV_COLOR_FORMAT_RGB565,

        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {.buff_dma = true, .swap_bytes = true, .buff_spiram = true}};
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);
}

void xiaozhi_lvgl_Init(void)
{
    lcd_Init();
    app_lvgl_init();
}

void lv_example_get_started_1(void)
{
    // 进入临界区
    lvgl_port_lock(0);

    // 搭建UI界面
    // LCD背景颜色
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xfff000), LV_PART_MAIN);
    title = lv_label_create(lv_screen_active());
    // 顶部标题
    lv_label_set_text(title, "please reset");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // 中间部分内容-表情内容.......
    emoji = lv_label_create(lv_screen_active());
    lv_label_set_text(emoji, "");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(emoji, LV_ALIGN_CENTER, 0, 0);

    // 底部内容
    dialog = lv_label_create(lv_screen_active());
    lv_label_set_text(dialog, "");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(dialog, LV_ALIGN_BOTTOM_MID, 0, -20);

    // 退出临界区
    lvgl_port_unlock();
}

// 6.封装修改标题的方法
void xiaozhi_lvgl_set_title(char *title1)
{
    // 进入临界区
    lvgl_port_lock(0);
    lv_label_set_text(title, title1);
    // 退出临界区
    lvgl_port_unlock();
}
// 7.封装修改emoji的方法
void xiaozhi_lvgl_set_emoji(char *emoji1)
{
    // 进入临界区
    lvgl_port_lock(0);
    lv_label_set_text(emoji, emoji1);
    // 退出临界区
    lvgl_port_unlock();
}
// 8修改dialog的方法
void xiaozhi_lvgl_set_dialog(char *dialog1)
{
    // 进入临界区
    lvgl_port_lock(0);
    lv_label_set_text(dialog, dialog1);
    // 退出临界区
    lvgl_port_unlock();
}

void xiaozhi_QR(char *data)
{
    // lv_color_t bg_color = lv_palette_lighten(LV_PALETTE_LIGHT_BLUE, 5);
    // lv_color_t fg_color = lv_palette_darken(LV_PALETTE_BLUE, 4);
    lvgl_port_lock(0);

    qr = lv_qrcode_create(lv_screen_active());
    lv_qrcode_set_size(qr, 150);
    lv_qrcode_set_dark_color(qr, lv_color_hex(0));
    lv_qrcode_set_light_color(qr, lv_color_hex(0xffffff));
    /*Set data*/
    lv_qrcode_update(qr, data, strlen(data));
    lv_obj_center(qr);
    /*Add a border with bg_color*/
    lv_obj_set_style_border_color(qr, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(qr, 5, 0);

    xiaozhi_lvgl_set_title("PLASE SCAN THE QR TO PEIWANG");
    lvgl_port_unlock();
}

void xiaozhi_delQR(void)
{
    lvgl_port_lock(0);

    lv_obj_del(qr);

    lvgl_port_unlock();
}
