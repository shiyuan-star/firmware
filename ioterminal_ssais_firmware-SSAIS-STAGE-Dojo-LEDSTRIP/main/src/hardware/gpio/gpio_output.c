/**
 * @file gpio_output.c
 * @brief GPIO类外设输出驱动 （板载LED指示灯、三色灯、屏幕电源管理、板载Digital output）
 * @version 1.0
 * @date 2023-12-14
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "gpio_peripheral.h"
#include "common.h"

static const char *TAG = "GPIO OUT";

static led_indicator_handle_t s_sysStateLedHandle = NULL;        ///< 系统状态灯操作句柄
static led_indicator_handle_t s_alarmLedHandle[4] = {NULL};      ///< 警示灯操作句柄分别为 [0] beep/blue [1] red [2]yellow [3]green
static led_indicator_handle_t s_digitalOutputHandle[2] = {NULL}; ///< 板载Digital output操作句柄分别为 [0]output 1 [1] output 2

/**
 * @brief GPIO外设初始化 默认电源引脚全关闭，防止初始化默认拉低导致三色灯闪烁蜂鸣
 */
void outputGpioInit()
{
    gpio_set_level(CONFIG_ALARM_LED_BEEP_PIN, !CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH);
    gpio_set_level(CONFIG_ALARM_LED_RED_PIN, !CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH);
    gpio_set_level(CONFIG_ALARM_LED_YELLOW_PIN, !CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH);
    gpio_set_level(CONFIG_ALARM_LED_GREEN_PIN, !CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH);
    gpio_set_level(CONFIG_SCREEN_POWER_ENABLE_PIN, !CONFIG_IS_POWER_ON_WHEN_LEVEL_HIGH);
    gpio_set_level(CONFIG_SYS_STATE_LED_PIN, !CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH);
    gpio_config_t ioConf = {};
    ioConf.intr_type = GPIO_INTR_DISABLE;
    ioConf.mode = GPIO_MODE_OUTPUT;
    ioConf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    ioConf.pull_up_en = GPIO_PULLDOWN_DISABLE;
    ioConf.pin_bit_mask = ((1ULL << CONFIG_SCREEN_POWER_ENABLE_PIN));
    gpio_config(&ioConf);
}

// 板载LED灯效定义

static const blink_step_t state_led_boot_blink[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 300},            // Step 1: Turn on LED for 300 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 300},           // Step 2: Turn off LED for 300 ms
    {LED_BLINK_HOLD, LED_STATE_ON, 300},            // Step 3: Turn on LED for 300 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 300},           // Step 4: Turn off LED for 300 ms
    {LED_BLINK_BREATHE, LED_STATE_25_PERCENT, 300}, // Step 5: Fade LED from on to off over 300 ms
    {LED_BLINK_STOP, 0, 0},                         // Step 6: Stop blinking
};

static const blink_step_t state_led_fast_blink_loop[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 500},  // step1: turn on LED 500 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 500}, // step2: turn off LED 500 ms
    {LED_BLINK_LOOP, 0, 0},               // step3: loop from step1
};

static const blink_step_t state_led_Slow_blink_loop[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 1500},  // step1: turn on LED 1500 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 1500}, // step2: turn off LED 1500 ms
    {LED_BLINK_LOOP, 0, 0},                // step3: loop from step1
};

static const blink_step_t state_led_breathe[] = {
    {LED_BLINK_BREATHE, LED_STATE_25_PERCENT, 1000}, // step1: fade from on to off 1300ms
    {LED_BLINK_BREATHE, LED_STATE_75_PERCENT, 800},  // step2: fade from off to on 800ms
    {LED_BLINK_LOOP, 0, 0},                          // step3: loop from step1
};

blink_step_t const *state_led_indicator_blink_lists[] = {
    [STATE_LED_BOOT_BLINK] = state_led_boot_blink,
    [STATE_LED_FAST_BLINK_LOOP] = state_led_fast_blink_loop,
    [STATE_LED_SLOW_BLINK_LOOP] = state_led_Slow_blink_loop,
    [STATE_LED_BREATHE] = state_led_breathe,
    [STATE_LED_MAX] = NULL,
};

/**
 * @brief 初始化板载指示灯初始化全局句柄
 * @return esp_err_t
 */
esp_err_t stateLedIndicatorInit()
{
    // 板载指示灯初始化部分
    led_indicator_ledc_config_t led_indicator_sys_state_led_ledc_config = {
        .is_active_level_high = CONFIG_IS_LED_ON_WHEN_LEVEL_HIGH,
        .timer_inited = ALARM_LED_OFF,
        .timer_num = LEDC_TIMER_0,
        .gpio_num = CONFIG_SYS_STATE_LED_PIN,
        .channel = LEDC_CHANNEL_0,
    };

    led_indicator_config_t sys_state_led_config = {
        .mode = LED_LEDC_MODE,
        .led_indicator_ledc_config = &led_indicator_sys_state_led_ledc_config,
        .blink_lists = state_led_indicator_blink_lists,
        .blink_list_num = STATE_LED_MAX,
    };
    s_sysStateLedHandle = led_indicator_create(&sys_state_led_config);
    if (s_sysStateLedHandle == NULL)
    {
        ESP_LOGE(TAG, "s_sysStateLedHandle create failed");
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(led_indicator_start(s_sysStateLedHandle, STATE_LED_BREATHE));            // 默认状态灯效
    ESP_ERROR_CHECK(led_indicator_preempt_start(s_sysStateLedHandle, STATE_LED_BOOT_BLINK)); // 抢占启动闪烁
    return ESP_OK;
}

// 三色灯灯效定义
static const blink_step_t alarm_led_off[] = {
    {LED_BLINK_HOLD, LED_STATE_OFF, 100}, // step1: turn off LED 100 ms
    {LED_BLINK_STOP, 0, 0},               // step2: stop
};

static const blink_step_t alarm_led_on[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 100}, // step1: turn on LED 100 ms
    {LED_BLINK_STOP, 0, 0},              // step2: stop
};

static const blink_step_t alarm_led_slow_blink_loop[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 800},  // step1: turn on LED 800 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 800}, // step2: turn off LED 800 ms
    {LED_BLINK_LOOP, 0, 0},               // step3: loop from step1
};

static blink_step_t const *alarm_led_indicator_blink_lists[] = {
    [ALARM_LED_OFF] = alarm_led_off,
    [ALARM_LED_ON] = alarm_led_on,
    [ALARM_LED_SLOW_BLINK_LOOP] = alarm_led_slow_blink_loop,
    [ALARM_LED_MAX] = NULL,
};

/**
 * @brief 初始化警示灯初始化全局句柄
 * @return esp_err_t
 */
esp_err_t alarmLedIndicatorInit()
{
    // 警示灯初始化部分
    led_indicator_gpio_config_t led_indicator_gpio_config[4] = {
        [0] = {
            .gpio_num = CONFIG_ALARM_LED_BEEP_PIN,
            .is_active_level_high = CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH,
        },
        [1] = {
            .gpio_num = CONFIG_ALARM_LED_RED_PIN,
            .is_active_level_high = CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH,
        },
        [2] = {
            .gpio_num = CONFIG_ALARM_LED_YELLOW_PIN,
            .is_active_level_high = CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH,
        },
        [3] = {
            .gpio_num = CONFIG_ALARM_LED_GREEN_PIN,
            .is_active_level_high = CONFIG_IS_LIGHT_ON_WHEN_LEVEL_HIGH,
        },

    };

    led_indicator_config_t alarm_led_config[4] = {
        [0] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&led_indicator_gpio_config[0],
            .blink_lists = alarm_led_indicator_blink_lists,
            .blink_list_num = ALARM_LED_MAX,
        },
        [1] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&led_indicator_gpio_config[1],
            .blink_lists = alarm_led_indicator_blink_lists,
            .blink_list_num = ALARM_LED_MAX,
        },
        [2] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&led_indicator_gpio_config[2],
            .blink_lists = alarm_led_indicator_blink_lists,
            .blink_list_num = ALARM_LED_MAX,
        },
        [3] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&led_indicator_gpio_config[3],
            .blink_lists = alarm_led_indicator_blink_lists,
            .blink_list_num = ALARM_LED_MAX,
        },
    };
    for (int i = 0; i < 4; i++)
    {
        s_alarmLedHandle[i] = led_indicator_create(&alarm_led_config[i]);
        if (s_alarmLedHandle[i] == NULL)
        {
            ESP_LOGE(TAG, "s_alarmLedHandle[%d] create failed", i);
            return ESP_FAIL;
        }
    }
    ESP_ERROR_CHECK(led_indicator_set_on_off(s_alarmLedHandle[0], false));
    ESP_ERROR_CHECK(led_indicator_set_on_off(s_alarmLedHandle[1], false));
    ESP_ERROR_CHECK(led_indicator_set_on_off(s_alarmLedHandle[2], false));
    ESP_ERROR_CHECK(led_indicator_set_on_off(s_alarmLedHandle[3], false));
    return ESP_OK;
}

/**
 * @brief 警示灯状态设置
 * @param alarmLedState 警示灯状态
 */
void alarmLedStateSet(AlarmLedState alarmLedState)
{
    if (alarmLedState <= ALARM_STATE_BRYG_1111)
    {
        if (alarmLedState & 0x01) // 绿灯
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[3], ALARM_LED_ON));
        }
        else
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[3], ALARM_LED_OFF));
        }
        if (alarmLedState & 0x02) // 黄灯
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[2], ALARM_LED_ON));
        }
        else
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[2], ALARM_LED_OFF));
        }
        if (alarmLedState & 0x04) // 红灯
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[1], ALARM_LED_ON));
        }
        else
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[1], ALARM_LED_OFF));
        }
        if (alarmLedState & 0x08) // 蜂鸣或者蓝色
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[0], ALARM_LED_ON));
        }
        else
        {
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[0], ALARM_LED_OFF));
        }
    }
    else
    {
        switch (alarmLedState)
        {
        case ALARM_STATE_NET_DISCONNECT: // 网络断开 红灯闪烁
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[0], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[1], ALARM_LED_SLOW_BLINK_LOOP));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[2], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[3], ALARM_LED_OFF));
            break;
        case ALARM_STATE_SYSTEAM_STAND_BY:
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[0], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[1], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[2], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[3], ALARM_LED_OFF));
            break;
        case ALARM_STATE_INDICATION_ERR:
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[0], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[1], ALARM_LED_OFF));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[2], ALARM_LED_SLOW_BLINK_LOOP));
            ESP_ERROR_CHECK(led_indicator_preempt_start(s_alarmLedHandle[3], ALARM_LED_OFF));
            break;
        default:
            ESP_LOGE(TAG, "alarmLedStateSet state err  alarmLedState = %d", alarmLedState);
            break;
        }
    }
}

static const blink_step_t digital_output_Slow_blink_loop[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 1500},  // step1: turn on LED 1500 ms
    {LED_BLINK_HOLD, LED_STATE_OFF, 1500}, // step2: turn off LED 1500 ms
    {LED_BLINK_LOOP, 0, 0},                // step3: loop from step1
};

static blink_step_t const *digital_output_lists[] = {
    [DIGITAL_OUTPUT_SLOW_BLINK_LOOP] = digital_output_Slow_blink_loop,
    [DIGITAL_OUTPUT_MAX] = NULL,
};

/**
 * @brief 初始化板载Digital output初始化全局句柄
 * @return esp_err_t
 */
esp_err_t digitalOutputIndicatorInit()
{
    // 板载DIO初始化部分
    led_indicator_gpio_config_t digital_output_gpio_config[4] = {
        [0] = {
            .gpio_num = CONFIG_DIGITAL_OUTPUT1_PIN,
            .is_active_level_high = CONFIG_IS_OUTPUT_ENABLE_WHEN_LEVEL_HIGH,
        },
        [1] = {
            .gpio_num = CONFIG_DIGITAL_OUTPUT2_PIN,
            .is_active_level_high = CONFIG_IS_OUTPUT_ENABLE_WHEN_LEVEL_HIGH,
        },

    };

    led_indicator_config_t digital_output_config[2] = {
        [0] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&digital_output_gpio_config[0],
            .blink_lists = digital_output_lists,
            .blink_list_num = DIGITAL_OUTPUT_MAX,
        },
        [1] = {
            .mode = LED_GPIO_MODE,
            .led_indicator_ledc_config = (led_indicator_ledc_config_t *)&digital_output_gpio_config[1],
            .blink_lists = digital_output_lists,
            .blink_list_num = DIGITAL_OUTPUT_MAX,
        },
    };
    for (int i = 0; i < 2; i++)
    {
        s_digitalOutputHandle[i] = led_indicator_create(&digital_output_config[i]);
        if (s_digitalOutputHandle[i] == NULL)
        {
            ESP_LOGE(TAG, "s_digitalOutputHandle[%d] create failed", i);
            return ESP_FAIL;
        }
    }
    ESP_ERROR_CHECK(led_indicator_start(s_digitalOutputHandle[0], DIGITAL_OUTPUT_SLOW_BLINK_LOOP));
    ESP_ERROR_CHECK(led_indicator_start(s_digitalOutputHandle[1], DIGITAL_OUTPUT_SLOW_BLINK_LOOP));
    return ESP_OK;
}