/**
 * @file protocol_set.c
 * @brief 协议设置页面处理
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#include "screen.h"

/**
 * @brief  协议设置页面处理
 * @param  controlId
 * @param  param
 * @return esp_err_t
 */
esp_err_t protocolSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_MQTT_SET_BUTTON:
        screenInfoUpdate(SCREEN_MQTT_SET_PAGE);
        break;
    case SCREEN_NTP_SET_BUTTON:
        screenInfoUpdate(SCREEN_NTP_SET_PAGE);
        break;
    case SCREEN_OTA_SET_BUTTON:
        screenInfoUpdate(SCREEN_OTA_SET_PAGE);
        break;
    default:
        break;
    }
    return ESP_OK;
}
