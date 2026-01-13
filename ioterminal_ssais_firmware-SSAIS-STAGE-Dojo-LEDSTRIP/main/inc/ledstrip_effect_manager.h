/**
 * @file led_strip_effect_manager.h
 * @brief 灯带效果管理器头文件 - 简化版，只支持单个效果
 * @version 1.1
 * @date 2025-04-25
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef LED_STRIP_EFFECT_MANAGER_H
#define LED_STRIP_EFFECT_MANAGER_H

#include <stdint.h>
#include <esp_err.h>
#include "cJSON.h"

/**
 * @brief 灯带效果类型枚举
 */
typedef enum
{
    LED_EFFECT_NONE = 0,       // 无效果
    LED_EFFECT_STATIC,         // 静态显示
    LED_EFFECT_BLINK,          // 闪烁
    LED_EFFECT_BREATHE,        // 呼吸
    LED_EFFECT_RAINBOW,        // 彩虹
    LED_EFFECT_CHASE,          // 追逐
    LED_EFFECT_GRADIENT,       // 渐变
    LED_EFFECT_FIRE,           // 火焰
    LED_EFFECT_WAVE,           // 波浪
    LED_EFFECT_RANDOM_TWINKLE, // 顺序跑马闪烁颜色
    LED_EFFECT_RANDOM_TWINKLE2, // 顺序跑马随机切换颜色
    LED_EFFECT_COMET,          // 彗星
    LED_EFFECT_MAX
} led_strip_effect_type_t;

/**
 * @brief 灯带效果参数结构体
 */
typedef struct
{
    led_strip_effect_type_t effect_type; // 效果类型
    uint16_t start_led;                  // 起始LED
    uint16_t end_led;                    // 结束LED
    uint8_t brightness;                  // 亮度
    uint32_t color1;                     // 主颜色
    uint32_t color2;                     // 辅助颜色(用于渐变等效果)
    uint16_t speed;                      // 速度(毫秒)
    uint8_t cycles;                      // 循环次数(0表示无限循环)
} led_strip_effect_params_t;

/**
 * @brief 运行灯带效果
 * 只支持一个效果，如果有效果正在运行，将会停止并释放旧效果
 *
 * @param params 效果参数
 * @return esp_err_t
 */
esp_err_t led_strip_effect_run(const led_strip_effect_params_t *params);

/**
 * @brief 停止当前正在运行的灯带效果
 *
 * @return esp_err_t
 */
esp_err_t led_strip_effect_stop(void);

/**
 * @brief 检查是否有效果正在运行
 *
 * @return bool 是否有效果正在运行
 */
bool led_strip_effect_is_running(void);

/**
 * @brief 处理MQTT请求以控制LED效果
 *
 * @param data cJSON数据对象
 * @return esp_err_t
 */
esp_err_t ledEffect(cJSON *data);

/**
 * @brief 灯带顺序跑马 携带颜色和亮度
 * param data cJSON数据对象
 * @return esp_err_t
 */
esp_err_t ledSequenceWithColorAndBrigh(cJSON *data);

/**
 * @brief 设定一段亮灯携带颜色和亮度
 * param data cJSON数据对象
 * @return esp_err_t
 */
esp_err_t ledLightUpWithColorAndBrigh(cJSON *data);

/**
 * @brief 关闭灯带
 * @return esp_err_t
 */
esp_err_t ledLightOff(void);

#endif // LED_STRIP_EFFECT_MANAGER_H