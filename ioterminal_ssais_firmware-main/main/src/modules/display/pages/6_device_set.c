/**
 * @file device_set.c
 * @brief 外设设置页面
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#include "screen.h"

/**
 * @brief  外设设置界面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t deviceSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_LED_STRIP_SET_BUTTON:
        screenInfoUpdate(SCREEN_LEDSTRIP_SET_PAGE);
        break;
    default:
        break;
    }
    return ESP_OK;
}
