/**
 * @file device_type.c
 * @brief MQTT操作设备命令处理
 * @version 1.0
 * @date 2024-04-07
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "network.h"

static char *TAG = "MQTT_DEVICE_CMD";

/**
 * @brief  MQTT设置外设设备
 * @param  mqttContorType
 * @param  mqttCmdType
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttSetDeviceHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data)
{
    switch (mqttContorType)
    {
    case MQTT_CONTROL_TYPE_DEVICE_ALARMLED:
        if (mqttCmdType == SET_ALARMLED_STATE)
        {
            cJSON *colorJson = getJSONobj(data, "color");
            if (colorJson == NULL)
            {
                return ESP_FAIL;
            }
            uint16_t color = cJSON_GetNumberValue(colorJson);
            if (color <= ALARM_STATE_BRYG_1111)
            {
                alarmLedStateSet(color);
            }
            else
            {
                return ESP_ERR_NOT_SUPPORTED;
            }
        }
        else
        {
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;
    default:
        return ESP_ERR_NOT_SUPPORTED;
        break;
    }
    return ESP_OK;
}
