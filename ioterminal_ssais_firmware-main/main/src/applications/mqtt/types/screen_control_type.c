/**
 * @file screen_control_type.c
 * @brief  MQTT操作屏幕控件命令处理
 * @version 1.0
 * @date 2024-01-31
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "network.h"

static char *TAG = "MQTT_SCREEN_CONTROL_CMD";

/**
 * @brief  MQTT设置屏幕控件内容
 * @param  mqttContorType
 * @param  mqttCmdType
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttSetScreenControlHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data)
{
    cJSON *screenIdJson = NULL;
    cJSON *controlIdJson = NULL;
    screenIdJson = getJSONobj(data, "screen_id");
    controlIdJson = getJSONobj(data, "control_id");
    uint16_t _screenId = 0;
    uint16_t _controlId = 0;
    if (screenIdJson != NULL && controlIdJson != NULL)
    {
        _screenId = cJSON_GetNumberValue(screenIdJson);
        _controlId = cJSON_GetNumberValue(controlIdJson);
    }
    else
    {
        return ESP_FAIL;
    }

    switch (mqttContorType)
    {
    case MQTT_CONTROL_TYPE_SCREEN_CONTROL_BUTTON:
        if (mqttCmdType == SET_BUTTON_VALUE)
        {
            cJSON *stateJson = getJSONobj(data, "state");
            if (stateJson == NULL)
            {
                return ESP_FAIL;
            }
            SetButtonValue(_screenId, _controlId, cJSON_GetNumberValue(stateJson));
        }
        break;
    case MQTT_CONTROL_TYPE_SCREEN_CONTROL_TEXT:
        if (mqttCmdType == SET_TEXT_VAULE)
        {
            cJSON *stringJson = getJSONobj(data, "string");
            if (stringJson == NULL)
            {
                return ESP_FAIL;
            }
            char *screenStrBuf = cJSON_GetStringValue(stringJson);
            if (screenStrBuf == NULL)
            {
                ESP_LOGE(TAG, "SET_TEXT_VAULE ERR : String is NULL");
                return ESP_FAIL;
            }
            SetTextValue(_screenId, _controlId, (uint8_t *)screenStrBuf);
        }
        else if (mqttCmdType == SET_TEXT_COLOR)
        {
            cJSON *rgbJson = getJSONobj(data, "rgb");
            if (rgbJson == NULL)
            {
                return ESP_FAIL;
            }
            SetTextColor(_screenId, _controlId, cJSON_GetNumberValue(rgbJson));
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
