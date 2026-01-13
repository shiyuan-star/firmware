/**
 * @file led_strip_effect_manager.c
 * @brief 灯带效果管理器实现 - 简化版，只支持单个效果
 * @version 1.1
 * @date 2025-04-25
 *
 * @copyright Copyright (c) 2025
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "led_strip.h"
#include "ledstrip.h"
#include "ledstrip_effect_manager.h"
#include "business.h"

static const char *TAG = "LED_EFFECT";

// 效果更新任务堆栈大小
#define EFFECT_TASK_STACK_SIZE 3072

// 效果更新任务优先级 (低优先级，不影响关键任务)
#define EFFECT_TASK_PRIORITY 2

// 效果更新间隔 (20ms = 50Hz 刷新率)
#define EFFECT_UPDATE_INTERVAL_MS 20

// 全局变量
static TaskHandle_t effect_task_handle = NULL;       // 效果任务句柄
static SemaphoreHandle_t effect_mutex = NULL;        // 效果互斥锁
static led_strip_handle_t _ledstripRmtHandle = NULL; // LED灯带句柄
static uint16_t led_count = 0;                       // LED数量
static bool task_running = false;                    // 任务运行标志
static bool effect_running = false;                  // 效果运行标志
static led_strip_effect_params_t current_effect;     // 当前效果参数
static uint32_t effect_start_time = 0;               // 效果开始时间
static uint32_t effect_frame_count = 0;              // 效果帧计数

// 前向声明
static void effect_task(void *arg);
static void initialize_manager(void);
static bool update_effect(uint32_t current_time);

/**
 * @brief 初始化效果管理器
 */
static void initialize_manager(void)
{
    if (effect_mutex == NULL)
    {
        // 创建效果互斥锁
        effect_mutex = xSemaphoreCreateMutex();
        if (effect_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create effect mutex");
            return;
        }
    }

    // 只有当LED灯带句柄为NULL时才重新获取
    if (_ledstripRmtHandle == NULL)
    {
        _ledstripRmtHandle = g_ledstripRmtHandle;
        if (_ledstripRmtHandle == NULL)
        {
            ESP_LOGE(TAG, "LED strip handle is NULL, cannot initialize effect manager");
            return;
        }
    }

    // 只有当LED数量为0时才重新获取
    if (led_count == 0)
    {
        led_count = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
        if (led_count == 0)
        {
            ESP_LOGE(TAG, "LED count is 0, cannot initialize effect manager");
            return;
        }
        ESP_LOGI(TAG, "LED effect manager initialized, LED count: %d", led_count);
    }
}

/**
 * @brief 运行灯带效果
 */
esp_err_t led_strip_effect_run(const led_strip_effect_params_t *params)
{
    if (params == NULL || params->effect_type >= LED_EFFECT_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    initialize_manager();

    if (effect_mutex == NULL)
    {
        return ESP_FAIL;
    }

    // 验证参数
    if (params->start_led == 0 || params->start_led > led_count ||
        params->end_led == 0 || params->end_led > led_count ||
        params->start_led > params->end_led)
    {
        ESP_LOGE(TAG, "Invalid LED range: %d-%d (max: %d)",
                 params->start_led, params->end_led, led_count);
        return ESP_ERR_INVALID_ARG;
    }

    // 获取互斥锁
    if (xSemaphoreTake(effect_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_FAIL;
    }

    // 如果有正在运行的效果，先停止它
    if (effect_running)
    {
        effect_running = false;
        xSemaphoreGive(effect_mutex);
        vTaskDelay(pdMS_TO_TICKS(50)); // 等待效果任务停止
        xSemaphoreTake(effect_mutex, portMAX_DELAY);
    }

    // 复制新效果参数
    memcpy(&current_effect, params, sizeof(led_strip_effect_params_t));

    // 重置效果状态
    effect_start_time = (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
    effect_frame_count = 0;
    effect_running = true;

    // 如果任务不存在，创建任务
    if (!task_running)
    {
        task_running = true;
        BaseType_t ret = xTaskCreate(
            effect_task,
            "led_effect_task",
            EFFECT_TASK_STACK_SIZE,
            NULL,
            EFFECT_TASK_PRIORITY,
            &effect_task_handle);

        if (ret != pdPASS)
        {
            ESP_LOGE(TAG, "Failed to create effect task");
            effect_running = false;
            task_running = false;
            xSemaphoreGive(effect_mutex);
            return ESP_FAIL;
        }
    }

    xSemaphoreGive(effect_mutex);

    ESP_LOGI(TAG, "Started effect type %d for LEDs %d-%d",
             params->effect_type, params->start_led, params->end_led);

    return ESP_OK;
}

/**
 * @brief 停止当前正在运行的灯带效果
 */
esp_err_t led_strip_effect_stop(void)
{
    if (effect_mutex == NULL || !effect_running)
    {
        return ESP_OK;
    }

    if (xSemaphoreTake(effect_mutex, portMAX_DELAY) == pdTRUE)
    {
        effect_running = false;
        xSemaphoreGive(effect_mutex);
        ESP_LOGI(TAG, "Stopped LED effect");
        return ESP_OK;
    }

    return ESP_FAIL;
}

/**
 * @brief 检查是否有效果正在运行
 */
bool led_strip_effect_is_running(void)
{
    bool running = false;

    if (effect_mutex != NULL)
    {
        if (xSemaphoreTake(effect_mutex, portMAX_DELAY) == pdTRUE)
        {
            running = effect_running;
            xSemaphoreGive(effect_mutex);
        }
    }

    return running;
}

/**
 * @brief 更新当前效果
 *
 * @param current_time 当前时间(毫秒)
 * @return true 如果效果仍在运行
 * @return false 如果效果已结束
 */
static bool update_effect(uint32_t current_time)
{
    if (!effect_running)
    {
        return false;
    }

    uint32_t elapsed_time = current_time - effect_start_time;
    uint16_t start_led = current_effect.start_led - 1; // 转为0基索引
    uint16_t end_led = current_effect.end_led - 1;
    uint8_t brightness = current_effect.brightness;
    uint16_t duration = current_effect.speed;
    uint8_t cycles = current_effect.cycles;

    // 检查是否完成所需循环
    if (cycles > 0)
    {
        uint32_t total_duration = duration * cycles;
        if (elapsed_time >= total_duration)
        {
            effect_running = false;

            // 清空所有LED
            for (uint16_t i = 0; i < led_count; i++)
            {
                led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
            }
            led_strip_refresh(_ledstripRmtHandle);

            ESP_LOGI(TAG, "Effect completed after %d cycles", cycles);
            return false;
        }
    }

    // 提取颜色分量
    uint8_t r1 = (current_effect.color1 >> 16) & 0xFF;
    uint8_t g1 = (current_effect.color1 >> 8) & 0xFF;
    uint8_t b1 = current_effect.color1 & 0xFF;

    uint8_t r2 = (current_effect.color2 >> 16) & 0xFF;
    uint8_t g2 = (current_effect.color2 >> 8) & 0xFF;
    uint8_t b2 = current_effect.color2 & 0xFF;

    // 亮度调整
    float bright_factor = brightness / 255.0f;

    // 根据效果类型更新LED
    switch (current_effect.effect_type)
    {
    case LED_EFFECT_STATIC:
        // 静态颜色显示
        for (uint16_t i = start_led; i <= end_led; i++)
        {
            led_strip_set_pixel(_ledstripRmtHandle, i,
                                r1 * bright_factor,
                                g1 * bright_factor,
                                b1 * bright_factor);
        }
        break;

    case LED_EFFECT_BLINK:
        // 闪烁效果
        {
            bool on = (elapsed_time % duration) < (duration / 2);

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                if (on)
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i,
                                        r1 * bright_factor,
                                        g1 * bright_factor,
                                        b1 * bright_factor);
                }
                else
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
                }
            }
        }
        break;

    case LED_EFFECT_BREATHE:
        // 呼吸效果 - 使用正弦波
        {
            float phase = (elapsed_time % duration) / (float)duration;
            float intensity = sinf(phase * 2 * M_PI) * 0.5f + 0.5f;

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                led_strip_set_pixel(_ledstripRmtHandle, i,
                                    r1 * intensity * bright_factor,
                                    g1 * intensity * bright_factor,
                                    b1 * intensity * bright_factor);
            }
        }
        break;

    case LED_EFFECT_RAINBOW:
        // 彩虹效果
        {
            for (uint16_t i = start_led; i <= end_led; i++)
            {
                float hue = fmodf(elapsed_time / 50.0f + (i - start_led) * 5, 360.0f);

                // ESP-IDF的led_strip组件可能不直接支持HSV，如果不支持，使用以下RGB转换
                float h = hue / 60.0f;
                float s = 1.0f;
                float v = bright_factor;

                int hi = (int)h % 6;
                float f = h - (int)h;
                float p = v * (1 - s);
                float q = v * (1 - s * f);
                float t = v * (1 - s * (1 - f));

                float r, g, b;

                if (hi == 0)
                {
                    r = v;
                    g = t;
                    b = p;
                }
                else if (hi == 1)
                {
                    r = q;
                    g = v;
                    b = p;
                }
                else if (hi == 2)
                {
                    r = p;
                    g = v;
                    b = t;
                }
                else if (hi == 3)
                {
                    r = p;
                    g = q;
                    b = v;
                }
                else if (hi == 4)
                {
                    r = t;
                    g = p;
                    b = v;
                }
                else
                {
                    r = v;
                    g = p;
                    b = q;
                }

                led_strip_set_pixel(_ledstripRmtHandle, i,
                                    r * 255.0f,
                                    g * 255.0f,
                                    b * 255.0f);
            }
        }
        break;

    case LED_EFFECT_CHASE:
        // 追逐效果
        {
            uint16_t led_count = end_led - start_led + 1;
            uint16_t pos = (elapsed_time / 50) % led_count;

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                if (i == start_led + pos)
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i,
                                        r1 * bright_factor,
                                        g1 * bright_factor,
                                        b1 * bright_factor);
                }
                else
                {
                    // 尾部逐渐变暗
                    uint16_t distance = (i > start_led + pos)
                                            ? (i - (start_led + pos))
                                            : (led_count - (start_led + pos - i));

                    if (distance <= 5)
                    {
                        float fade = (5 - distance) / 5.0f;
                        led_strip_set_pixel(_ledstripRmtHandle, i,
                                            r1 * fade * bright_factor,
                                            g1 * fade * bright_factor,
                                            b1 * fade * bright_factor);
                    }
                    else
                    {
                        led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
                    }
                }
            }
        }
        break;

    case LED_EFFECT_GRADIENT:
        // 颜色渐变
        {
            float phase = (elapsed_time % duration) / (float)duration;

            // 在两个颜色之间渐变
            uint8_t r = r1 * (1 - phase) + r2 * phase;
            uint8_t g = g1 * (1 - phase) + g2 * phase;
            uint8_t b = b1 * (1 - phase) + b2 * phase;

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                led_strip_set_pixel(_ledstripRmtHandle, i,
                                    r * bright_factor,
                                    g * bright_factor,
                                    b * bright_factor);
            }
        }
        break;

    case LED_EFFECT_FIRE:
        // 火焰效果
        {
            for (uint16_t i = start_led; i <= end_led; i++)
            {
                // 基于位置和时间的随机火焰
                float rand_val = (sinf(i * 1337.0f + elapsed_time * 0.01f) * 0.5f + 0.5f);
                float intensity = rand_val * rand_val * bright_factor; // 更强调明亮部分

                // 火焰颜色从黄到红
                uint8_t r = 255 * intensity;
                uint8_t g = 100 * intensity * rand_val; // 绿色分量随机变化
                uint8_t b = 0;

                led_strip_set_pixel(_ledstripRmtHandle, i, r, g, b);
            }
        }
        break;

    case LED_EFFECT_WAVE:
        // 波浪效果
        {
            float wave_speed = 2.0f; // 波速
            float wave_width = 0.5f; // 波宽

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                float phase = fmodf((i - start_led) / (float)(end_led - start_led) -
                                        elapsed_time * 0.001f * wave_speed,
                                    1.0f);

                // 创建一个平滑的波
                float intensity = (sinf(phase * 2 * M_PI) * 0.5f + 0.5f) *
                                  (sinf(phase * 2 * M_PI / wave_width) * 0.5f + 0.5f);

                led_strip_set_pixel(_ledstripRmtHandle, i,
                                    r1 * intensity * bright_factor,
                                    g1 * intensity * bright_factor,
                                    b1 * intensity * bright_factor);
            }
        }
        break;

    case LED_EFFECT_RANDOM_TWINKLE:
        // 随机闪烁效果 - 使用多种颜色
        {
            // 定义四种固定颜色: 绿色、黄色、红色、蓝色
            const uint32_t colors[4] = {
                0x00FF00, // 绿色: 65280
                0xFFCC00, // 黄色: 16766720
                0xCD0000, // 红色: 13434880
                0x09F3F8  // 蓝色: 652280
            };

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                // 使用正弦函数生成伪随机数
                float rand_val = sinf(i * 1337.0f + elapsed_time * 0.01f) * 0.5f + 0.5f;

                // 第二个随机值，用于颜色选择
                float rand_color = sinf(i * 7919.0f + elapsed_time * 0.007f) * 0.5f + 0.5f;

                // 选择颜色索引 (0-3)
                int color_idx = (int)(rand_color * 4) % 4;

                // 提取选定颜色的RGB分量
                uint8_t r = (colors[color_idx] >> 16) & 0xFF;
                uint8_t g = (colors[color_idx] >> 8) & 0xFF;
                uint8_t b = colors[color_idx] & 0xFF;

                // 只有部分LED亮起
                if (rand_val > 0.8f)
                {
                    float intensity = (rand_val - 0.8f) * 5.0f; // 映射到0-1范围
                    led_strip_set_pixel(_ledstripRmtHandle, i,
                                        r * intensity * bright_factor,
                                        g * intensity * bright_factor,
                                        b * intensity * bright_factor);
                }
                else
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
                }
            }
        }
        break;

    case LED_EFFECT_RANDOM_TWINKLE2:
        // 随机闪烁效果2 - 定时切换单一颜色
        {
            // 定义四种固定颜色: 绿色、黄色、红色、蓝色
            const uint32_t colors[4] = {
                0x00FF00, // 绿色: 65280
                0xFFCC00, // 黄色: 16766720
                0x09F3F8, // 蓝色: 652280
                0xFF0000  // 红色: 16711680
            };

            // 根据经过的时间确定当前颜色索引
            // duration参数控制切换颜色的时间间隔
            int color_idx = (elapsed_time / duration) % 4;

            // 提取当前时间段对应颜色的RGB分量
            uint8_t r = (colors[color_idx] >> 16) & 0xFF;
            uint8_t g = (colors[color_idx] >> 8) & 0xFF;
            uint8_t b = colors[color_idx] & 0xFF;

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                // 使用正弦函数生成伪随机数，控制哪些LED会亮起
                float rand_val = sinf(i * 1337.0f + elapsed_time * 0.01f) * 0.5f + 0.5f;

                // 只有部分LED亮起 (与原效果保持一致，约20%的LED亮起)
                if (rand_val > 0.8f)
                {
                    float intensity = (rand_val - 0.8f) * 5.0f; // 映射到0-1范围
                    led_strip_set_pixel(_ledstripRmtHandle, i,
                                        r * intensity * bright_factor,
                                        g * intensity * bright_factor,
                                        b * intensity * bright_factor);
                }
                else
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
                }
            }
        }
        break;

    case LED_EFFECT_COMET:
        // 彗星效果
        {
            uint16_t led_count = end_led - start_led + 1;
            uint16_t comet_size = led_count / 4;
            if (comet_size < 1)
                comet_size = 1;

            // 彗星位置（往返移动）
            uint32_t cycle_time = duration / 2;
            uint32_t cycle_position = (elapsed_time % duration);
            bool reverse = cycle_position >= cycle_time;

            float pos;
            if (!reverse)
            {
                pos = (cycle_position / (float)cycle_time) * (led_count - 1);
            }
            else
            {
                pos = ((duration - cycle_position) / (float)cycle_time) * (led_count - 1);
            }

            for (uint16_t i = start_led; i <= end_led; i++)
            {
                float distance = fabsf(i - (start_led + pos));

                if (distance < comet_size)
                {
                    // 彗星头部最亮，尾部渐暗
                    float intensity = (comet_size - distance) / comet_size;
                    led_strip_set_pixel(_ledstripRmtHandle, i,
                                        r1 * intensity * bright_factor,
                                        g1 * intensity * bright_factor,
                                        b1 * intensity * bright_factor);
                }
                else
                {
                    led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
                }
            }
        }
        break;

    default:
        // 未知效果，不做任何处理
        break;
    }

    effect_frame_count++;
    return true;
}

/**
 * @brief 效果更新任务
 */
static void effect_task(void *arg)
{
    ESP_LOGI(TAG, "Effect task started");

    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t current_time;
    bool running = true;

    while (task_running)
    {
        current_time = (uint32_t)xTaskGetTickCount() * portTICK_PERIOD_MS;
        running = false;

        // 清空所有LED
        for (uint16_t i = 0; i < led_count; i++)
        {
            led_strip_set_pixel(_ledstripRmtHandle, i, 0, 0, 0);
        }

        // 更新效果
        if (xSemaphoreTake(effect_mutex, portMAX_DELAY) == pdTRUE)
        {
            if (effect_running && _ledstripRmtHandle != NULL)
            {
                running = update_effect(current_time);
            }
            xSemaphoreGive(effect_mutex);
        }

        // 刷新LED灯带
        if (_ledstripRmtHandle != NULL)
        {
            led_strip_refresh(_ledstripRmtHandle);
        }

        // 如果没有运行的效果和处理中的效果，退出任务
        if (!running && xSemaphoreTake(effect_mutex, portMAX_DELAY) == pdTRUE)
        {
            if (!effect_running)
            {
                task_running = false;
            }
            xSemaphoreGive(effect_mutex);
        }

        // 定时等待，确保一致的更新频率
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(EFFECT_UPDATE_INTERVAL_MS));
    }
    ESP_LOGI(TAG, "Effect task stopped");
    effect_task_handle = NULL;
    vTaskDelete(NULL);
}

/**
 * @brief MQTT命令处理LED效果
 *
 * @param data JSON数据
 * @return esp_err_t
 */
esp_err_t ledEffect(cJSON *data)
{
    if (data == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *effect_type_json = getJSONobj(data, "effect_type");
    cJSON *start_led_json = getJSONobj(data, "start_led");
    cJSON *end_led_json = getJSONobj(data, "end_led");
    cJSON *brightness_json = getJSONobj(data, "brightness");

    if (effect_type_json == NULL || start_led_json == NULL ||
        end_led_json == NULL || brightness_json == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    led_strip_effect_params_t params = {0};
    params.effect_type = (led_strip_effect_type_t)cJSON_GetNumberValue(effect_type_json);
    params.start_led = (uint16_t)cJSON_GetNumberValue(start_led_json);
    params.end_led = (uint16_t)cJSON_GetNumberValue(end_led_json);
    params.brightness = (uint8_t)cJSON_GetNumberValue(brightness_json);

    // 可选参数
    cJSON *color1_json = getJSONobj(data, "color1");
    if (color1_json != NULL)
    {
        params.color1 = (uint32_t)cJSON_GetNumberValue(color1_json);
    }
    else
    {
        params.color1 = 0xFFFFFF; // 默认白色
    }

    cJSON *color2_json = getJSONobj(data, "color2");
    if (color2_json != NULL)
    {
        params.color2 = (uint32_t)cJSON_GetNumberValue(color2_json);
    }
    else
    {
        params.color2 = 0x0000FF; // 默认蓝色
    }

    cJSON *speed_json = getJSONobj(data, "speed");
    if (speed_json != NULL)
    {
        params.speed = (uint16_t)cJSON_GetNumberValue(speed_json);
    }
    else
    {
        params.speed = 1000; // 默认1秒
    }

    cJSON *cycles_json = getJSONobj(data, "cycles");
    if (cycles_json != NULL)
    {
        params.cycles = (uint8_t)cJSON_GetNumberValue(cycles_json);
    }
    else
    {
        params.cycles = 0; // 默认无限循环
    }

    // 运行效果
    esp_err_t ret = led_strip_effect_run(&params);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to run effect");
        return ret;
    }

    ESP_LOGI(TAG, "Started effect %d for LEDs %d-%d",
             params.effect_type, params.start_led, params.end_led);

    return ESP_OK;
}

/**
 * @brief  灯珠顺序跑马 携带颜色和亮度
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledSequenceWithColorAndBrigh(cJSON *data)
{
    cJSON *_startLedJson = getJSONobj(data, "start_led");
    cJSON *_endLedJson = getJSONobj(data, "end_led");
    cJSON *_colorJson = getJSONobj(data, "color");
    cJSON *_brightnessJson = getJSONobj(data, "brightness");
    cJSON *_delayJson = getJSONobj(data, "delay");
    if (_startLedJson == NULL || _endLedJson == NULL || _colorJson == NULL || _brightnessJson == NULL || _delayJson == NULL)
    {
        return ESP_FAIL;
    }
    uint16_t _startLed = cJSON_GetNumberValue(_startLedJson);
    uint16_t _endLed = cJSON_GetNumberValue(_endLedJson);
    uint32_t _color = cJSON_GetNumberValue(_colorJson);
    uint16_t _brightness = cJSON_GetNumberValue(_brightnessJson);
    uint32_t _delay = cJSON_GetNumberValue(_delayJson);
    uint8_t _red = ((_color >> 16) & 0XFF);
    uint8_t _green = ((_color >> 8) & 0XFF);
    uint8_t _blue = (_color & 0XFF);
    float hue;          // hue (0 - 360)
    uint8_t saturation; // saturation (0 - 255)
    uint8_t value;      // value (0 - 255)
    RGB8882HSV(_red, _green, _blue, &hue, &saturation, &value);
    value = _brightness;
    LEDSTRIP_CLEAR;
    if (_delay == 0)
    {
        for (size_t i = _startLed; i <= _endLed; i++)
        {
            led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
            LEDSTRIP_REFRESH;
        }
    }
    else
    {
        for (size_t i = _startLed; i <= _endLed; i++)
        {
            led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
            LEDSTRIP_REFRESH;
            vTaskDelay(pdMS_TO_TICKS(_delay));
        }
    }
    LEDSTRIP_CLEAR;
    return ESP_OK;
}

/**
 * @brief  灯珠设定颜色和亮度
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledLightUpWithColorAndBrigh(cJSON *data)
{
    cJSON *_startLedJson = getJSONobj(data, "start_led");
    cJSON *_endLedJson = getJSONobj(data, "end_led");
    cJSON *_colorJson = getJSONobj(data, "color");
    cJSON *_brightnessJson = getJSONobj(data, "brightness");
    if (_startLedJson == NULL || _endLedJson == NULL || _colorJson == NULL || _brightnessJson == NULL)
    {
        return ESP_FAIL;
    }
    uint16_t _startLed = cJSON_GetNumberValue(_startLedJson);
    uint16_t _endLed = cJSON_GetNumberValue(_endLedJson);
    uint32_t _color = cJSON_GetNumberValue(_colorJson);
    uint16_t _brightness = cJSON_GetNumberValue(_brightnessJson);
    uint8_t _red = ((_color >> 16) & 0XFF);
    uint8_t _green = ((_color >> 8) & 0XFF);
    uint8_t _blue = (_color & 0XFF);
    float hue;          // hue (0 - 360)
    uint8_t saturation; // saturation (0 - 255)
    uint8_t value;      // value (0 - 255)
    RGB8882HSV(_red, _green, _blue, &hue, &saturation, &value);
    value = _brightness;
    LEDSTRIP_CLEAR;
    for (size_t i = _startLed; i <= _endLed; i++)
    {
        led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
    }
    LEDSTRIP_REFRESH;
    return ESP_OK;
}

/**
 * @brief 关闭所有灯
 */
esp_err_t ledLightOff(void)
{
    LEDSTRIP_CLEAR;
    LEDSTRIP_REFRESH;
    return ESP_OK;
}
