/**
 * @file project_set.c
 * @brief 项目设置界面处理
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#include "screen.h"

/**
 * @brief  项目设置界面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t projectSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SSAIS_LEDSTRIP_BUTTON:
        screenInfoUpdate(SCREEN_SSAIS_PROJECT_SET_PAGE);
        break;    
    default:
        break;
    }
    return ESP_OK;
}
