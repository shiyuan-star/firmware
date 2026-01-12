/**
 * @file screen_state_type.c
 * @brief MQTT操作屏幕状态命令处理
 * @version 1.0
 * @date 2024-04-07
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "network.h"

static char *TAG = "MQTT_SCREEN_STATE_CMD";

/**
 * @brief  MQTT设置屏幕状态
 * @param  mqttContorType
 * @param  mqttCmdType
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttSetScreenStateHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data)
{
    switch (mqttContorType)
    {
    case 1:
        break;
    default:
        return ESP_ERR_NOT_SUPPORTED;
        break;
    }
    return ESP_OK;
}
