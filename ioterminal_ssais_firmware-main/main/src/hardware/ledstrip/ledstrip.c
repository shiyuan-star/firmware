/**
 * @file ledstrip.c
 * @brief led灯带驱动
 * @version 1.0
 * @date 2024-02-05
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "ledstrip.h"

static const char *TAG = "LEDSTRIP";

/**
 * @brief  led灯带初始化
 * @param  ledstripConfigData
 * @return led_strip_handle_t
 */
led_strip_handle_t LedStripInit(LedstripConfigData_t ledstripConfigData)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_LED_STRIP_PIN,   // The GPIO that connected to the LED strip's data line
        .max_leds = ledstripConfigData.ledNum,    // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = ledstripConfigData.ledModel, // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
        .rmt_channel = 0,
#else
        .mem_block_symbols = 2046,
        .clk_src = RMT_CLK_SRC_DEFAULT,      // different clock source can lead to different power consumption
        .resolution_hz = (10 * 1000 * 1000), // RMT counter clock frequency 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
        .flags.with_dma = true,              // DMA feature is available on ESP target like ESP32-S3
#endif
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}
