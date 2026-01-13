/**
 * @file gpio_peripheral.h
 * @brief GPIO 外设头文件
 * @version 1.0
 * @date 2024-01-04
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */

#ifndef _GPIO_PERIPHERAL_H_
#define _GPIO_PERIPHERAL_H_

#include "led_indicator.h"
#include "iot_button.h"
#include "driver/gpio.h"
#include "data_type.h"

// 外部函数
extern void outputGpioInit();
extern void inputGpioInit();
extern void initDinButton();
extern void alarmLedStateSet(AlarmLedState alarmLedState);
extern esp_err_t stateLedIndicatorInit();
extern esp_err_t alarmLedIndicatorInit();
extern esp_err_t digitalOutputIndicatorInit();

#endif //_GPIO_PERIPHERAL_H_