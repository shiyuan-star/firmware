/**
 * @file mqtt_set.c
 * @brief MQTT设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  MQTT设置页面处理
 * @param  controlId
 * @param  param
 * @return esp_err_t
 */
esp_err_t mqttSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static MqttConfigData_t mqttConfigData;
    if (!initialized)
    {
        mqttConfigData = g_nvsData.networkConfigData.mqttConfigData;
        initialized = true;
    }

    // 根据控件ID，修改相应的配置数据

    switch (controlId)
    {
    case SCREEN_MQTT_ENABLE_BUTTON:
        mqttConfigData.mqttEnabled = param[1];
        break;
    case SCRENN_MQTT_SERVER_URL_TEXT:
        strcpy(mqttConfigData.url, (char *)param);
        break;
    case SCRENN_MQTT_USERNAME_TEXT:
        strcpy(mqttConfigData.userName, (char *)param);
        break;
    case SCRENN_MQTT_PASS_TEXT:
        strcpy(mqttConfigData.userPassword, (char *)param);
        break;
    case SCRENN_MQTT_SUBTOPIC_QOS_TEXT:
        mqttConfigData.subQos = str2num((char *)param);
        break;
    case SCRENN_MQTT_PUBTOPIC_QOS_TEXT:
        mqttConfigData.pubQos = str2num((char *)param);
        break;
    case SCREEN_MQTT_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&mqttConfigData, &(g_nvsData.networkConfigData.mqttConfigData), MqttConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.networkConfigData.mqttConfigData = mqttConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
