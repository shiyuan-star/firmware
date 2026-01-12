/**
 * @file gpio_input.c
 * @brief GPIO类外设输入驱动
 * @version 1.0
 * @date 2024-04-03
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "gpio_peripheral.h"
#include "common.h"
#include "unity.h"

static const char *TAG = "GPIO IN";
static button_handle_t g_InputBtns[2] = {0}; ///< DIN操作句柄分别为 [0] DIN1 [1] DIN2

/**
 * @brief GPIO输入类硬件初始化
 */
void inputGpioInit()
{
    gpio_config_t ioConf = {};
    ioConf.intr_type = GPIO_INTR_DISABLE;
    ioConf.mode = GPIO_MODE_INPUT;
    ioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConf.pull_up_en = GPIO_PULLUP_DISABLE;
    ioConf.pin_bit_mask = ((1ULL << CONFIG_DIGITAL_INPUT1_PIN) | (1ULL << CONFIG_DIGITAL_INPUT2_PIN));
    gpio_config(&ioConf);
}

static int get_btn_index(button_handle_t btn)
{
    for (size_t i = 0; i < 2; i++)
    {
        if (btn == g_InputBtns[i])
        {
            return i;
        }
    }
    return -1;
}

static void button_press_down_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_PRESS_DOWN, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_DOWN", get_btn_index((button_handle_t)arg));
}

static void button_press_up_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_PRESS_UP, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_UP[%d]", get_btn_index((button_handle_t)arg), iot_button_get_ticks_time((button_handle_t)arg));
}

static void button_press_repeat_cb(void *arg, void *data)
{
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_REPEAT[%d]", get_btn_index((button_handle_t)arg), iot_button_get_repeat((button_handle_t)arg));
}

static void button_single_click_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_SINGLE_CLICK, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_SINGLE_CLICK", get_btn_index((button_handle_t)arg));
}

static void button_double_click_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_DOUBLE_CLICK, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_DOUBLE_CLICK", get_btn_index((button_handle_t)arg));
}

static void button_long_press_start_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_LONG_PRESS_START, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_START", get_btn_index((button_handle_t)arg));
}

static void button_long_press_hold_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_LONG_PRESS_HOLD, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_LONG_PRESS_HOLD[%d],count is [%d]", get_btn_index((button_handle_t)arg), iot_button_get_ticks_time((button_handle_t)arg), iot_button_get_long_press_hold_cnt((button_handle_t)arg));
}

static void button_press_repeat_done_cb(void *arg, void *data)
{
    TEST_ASSERT_EQUAL_HEX(BUTTON_PRESS_REPEAT_DONE, iot_button_get_event(arg));
    ESP_LOGI(TAG, "BTN%d: BUTTON_PRESS_REPEAT_DONE[%d]", get_btn_index((button_handle_t)arg), iot_button_get_repeat((button_handle_t)arg));
}

void initDinButton()
{
    button_config_t cfg = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = CONFIG_DIGITAL_INPUT1_PIN,
            .active_level = CONFIG_IS_IO_LEVEL_HIGH_WHEN_INPUT_GROUNDING,
        }};
    g_InputBtns[0] = iot_button_create(&cfg);
    TEST_ASSERT_NOT_NULL(g_InputBtns[0]);
    iot_button_register_cb(g_InputBtns[0], BUTTON_PRESS_DOWN, button_press_down_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_PRESS_UP, button_press_up_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_PRESS_REPEAT, button_press_repeat_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_LONG_PRESS_START, button_long_press_start_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_LONG_PRESS_HOLD, button_long_press_hold_cb, NULL);
    iot_button_register_cb(g_InputBtns[0], BUTTON_PRESS_REPEAT_DONE, button_press_repeat_done_cb, NULL);

    cfg.gpio_button_config.gpio_num = CONFIG_DIGITAL_INPUT2_PIN;
    g_InputBtns[1] = iot_button_create(&cfg);
    TEST_ASSERT_NOT_NULL(g_InputBtns[1]);
    // iot_button_register_cb(g_InputBtns[1], BUTTON_PRESS_DOWN, button_press_down_cb, NULL);
    // iot_button_register_cb(g_InputBtns[1], BUTTON_PRESS_UP, button_press_up_cb, NULL);
    // iot_button_register_cb(g_InputBtns[1], BUTTON_PRESS_REPEAT, button_press_repeat_cb, NULL);
    iot_button_register_cb(g_InputBtns[1], BUTTON_SINGLE_CLICK, button_single_click_cb, NULL);
    iot_button_register_cb(g_InputBtns[1], BUTTON_DOUBLE_CLICK, button_double_click_cb, NULL);
    iot_button_register_cb(g_InputBtns[1], BUTTON_LONG_PRESS_START, button_long_press_start_cb, NULL);
    // iot_button_register_cb(g_InputBtns[1], BUTTON_LONG_PRESS_HOLD, button_long_press_hold_cb, NULL);
    // iot_button_register_cb(g_InputBtns[1], BUTTON_PRESS_REPEAT_DONE, button_press_repeat_done_cb, NULL);
}
