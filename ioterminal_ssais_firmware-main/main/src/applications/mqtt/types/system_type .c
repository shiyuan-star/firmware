/**
 * @file system_type .c
 * @brief MQTT操作系统命令
 * @version 1.0
 * @date 2024-04-07
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "network.h"

static char *TAG = "MQTT_SYSTEM_CMD";

/**
 * @brief  MQTT操作系统处理函数
 * @param  mqttContorType
 * @param  mqttCmdType
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttSetSystemHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data)
{
    switch (mqttContorType)
    {
    case MQTT_CONTROL_TYPE_SYSTEM_REBOOT:
        if (MQTT_SET_SYSTEM_REBOOT)
        {
            esp_restart();
        }
        else
        {
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;
    case MQTT_CONTROL_TYPE_SYSTEM_OTA:
        if (mqttCmdType == START_OTA_FROM_NVS_URL)
        {
            xSemaphoreGive(g_startOtaTaskSemphHandle);
        }
        else if (mqttCmdType == START_OTA_FROM_DATA_URL)
        {
            cJSON *urlJson = getJSONobj(data, "url");
            if (urlJson == NULL)
            {
                return ESP_FAIL;
            }
            char *urlStrBuf = cJSON_GetStringValue(urlJson);
            if (urlStrBuf == NULL)
            {
                ESP_LOGE(TAG, "OTA URL is NULL");
                return ESP_FAIL;
            }
            strcpy(g_nvsData.networkConfigData.otaConfigData.esp32OtaServer1, urlStrBuf);
            saveConfigToNvs(&g_nvsData);
            xSemaphoreGive(g_startOtaTaskSemphHandle);
        }
        else if (mqttCmdType == GET_OTA_NVS_URL) // 查询NVS存储的OTA地址链接
        {
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_FIRMWARE_VERSION, "url", g_nvsData.networkConfigData.otaConfigData.esp32OtaServer1);
        }
        else if (mqttCmdType == GET_FIRMWARE_VERSION) // 查询固件版本
        {
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_OTA, NOTIFY_FIRMWARE_VERSION, "version", FIRMWARE_VERSION);
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
