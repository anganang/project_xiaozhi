#ifndef __XIAOZHI_LCD_H__
#define __XIAOZHI_LCD_H__

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "pretty_effect.h"
#include "jpeg_decoder.h"
#include "string.h"

// Using SPI2 in the example, as it also supports octal modes on some targets
#define LCD_HOST SPI2_HOST
// To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many.
// More means more memory use, but less overhead for setting up / finishing transfers. Make sure 240
// is dividable by this.
#define PARALLEL_LINES 16
// The number of frames to show before rotate the graph
#define ROTATE_FRAME 0

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (20 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL 1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0 48 /*!< for 1-line SPI, this also refereed as MOSI */
#define EXAMPLE_PIN_NUM_PCLK 47
#define EXAMPLE_PIN_NUM_CS 21
#define EXAMPLE_PIN_NUM_DC 45
#define EXAMPLE_PIN_NUM_RST 16
#define EXAMPLE_PIN_NUM_BK_LIGHT 40

// 水平和垂直方向的像素数
#define EXAMPLE_LCD_H_RES 320
#define EXAMPLE_LCD_V_RES 240
// 用于表示命令和参数的位号
#define EXAMPLE_LCD_CMD_BITS 8
#define EXAMPLE_LCD_PARAM_BITS 8

void lcd_Init(void);

// void get_image(void);

// void send_image(uint8_t *image);

#endif /* __XIAOZHI_LCD_H__ */
