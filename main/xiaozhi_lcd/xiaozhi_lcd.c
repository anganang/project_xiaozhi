#include "xiaozhi_lcd.h"

// 请参考二进制文件中包含的 jpeg 文件
extern const uint8_t image_jpg_start[] asm("_binary_image_jpg_start");
extern const uint8_t image_jpg_end[] asm("_binary_image_jpg_end");

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

// static uint16_t *s_lines[2];

void lcd_Init()
{
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT};
    // Initialize the GPIO of backlight
    gpio_config(&bk_gpio_config);

    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_PCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_DATA0,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = PARALLEL_LINES * EXAMPLE_LCD_H_RES * 2 + 8};

    // Initialize the SPI bus
    spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };

    // 将LCD连接到SPI总线
    esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle);

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    // Initialize the LCD configuration
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);

    // 在初始化 LCD 面板驱动程序时，请关闭背光，以避免 LCD 屏幕显示异常。
    // （不同型号的 LCD 屏幕可能需要不同的亮度级别）
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    // 重置显示屏
    esp_lcd_panel_reset(panel_handle);

    // 初始化LCD面板
    esp_lcd_panel_init(panel_handle);

    // 打开屏幕
    esp_lcd_panel_disp_on_off(panel_handle, true);
    esp_lcd_panel_invert_color(panel_handle, false);

    // 交换 x 轴和 y 轴（不同 LCD 屏幕可能需要不同的选项）
    esp_lcd_panel_swap_xy(panel_handle, true);

    // 打开背光（不同液晶屏可能需要不同的亮度级别）
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    // // 给存储图像的缓冲区开辟空间
    // for (int i = 0; i < 2; i++)
    // {
    //     s_lines[i] = heap_caps_malloc(EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    //     // 这是一个断言检查，确保 s_lines[i] 不是空指针，它不替代正式的错误处理，只是开发期的安全网。
    //     // assert(s_lines[i] != NULL);
    // }
}

// void get_image(void)
// {

//     uint32_t size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t);
//     // 为图像开辟了一个空间
//     uint8_t *image_ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

//     // JPEG 解码配置
//     esp_jpeg_image_cfg_t jpeg_cfg = {
//         .indata = (uint8_t *)image_jpg_start,
//         .indata_size = image_jpg_end - image_jpg_start,
//         .outbuf = image_ptr,
//         .outbuf_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t),
//         .out_format = JPEG_IMAGE_FORMAT_RGB565,
//         .out_scale = JPEG_IMAGE_SCALE_0,
//         .flags = {
//             .swap_color_bytes = 1,
//         }};

//     // JPEG解码
//     esp_jpeg_image_output_t outimg;
//     esp_jpeg_decode(&jpeg_cfg, &outimg);

//     send_image(image_ptr);
// }

// void send_image(uint8_t *image)
// {
//     // 将驱动程序配置设置为每次旋转 180 度
//     esp_lcd_panel_mirror(panel_handle, true, false);

//     // 当前发送至LCD的行索引以及正在计算的行索引
//     int sending_line = 0;
//     int calc_line = 0;

//     for (int y = 0; y < EXAMPLE_LCD_V_RES; y += PARALLEL_LINES)
//     {
//         memcpy(s_lines[calc_line], image, EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t));
//         // 每发送完数据指针就要进行移动
//         image += EXAMPLE_LCD_H_RES * PARALLEL_LINES * sizeof(uint16_t);
//         sending_line = calc_line;
//         calc_line = !calc_line;
//         // 发送计算结果
//         esp_lcd_panel_draw_bitmap(panel_handle, 0, y, 0 + EXAMPLE_LCD_H_RES, y + PARALLEL_LINES, s_lines[sending_line]);
//     }
// }
