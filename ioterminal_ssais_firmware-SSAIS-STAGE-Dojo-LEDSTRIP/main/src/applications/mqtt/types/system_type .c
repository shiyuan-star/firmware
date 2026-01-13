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
 * @brief 报告NVS存储的OTA参数
 */
void mqttPubOtaParameterMsg()
{
    MqttPublishData_t _mqttPubData;
    cJSON *msgBuff = NULL;
    msgBuff = cJSON_CreateObject();
    cJSON_AddNumberToObject(msgBuff, "control_type", MQTT_CONTROL_TYPE_SYSTEM_OTA);
    cJSON_AddNumberToObject(msgBuff, "notify_type", NOTIFY_OTA_NVS_PARAMETER);
    cJSON *data = NULL;
    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, "url", g_nvsData.networkConfigData.otaConfigData.esp32OtaServer);
    cJSON_AddStringToObject(data, "file_name", g_nvsData.networkConfigData.otaConfigData.firmwareFileName);
    cJSON_AddItemToObject(msgBuff, "data", data);
    char *jsonStr = NULL;
    jsonStr = cJSON_PrintUnformatted(msgBuff);
    _mqttPubData.dataLen = strlen(jsonStr);
    strcpy(_mqttPubData.data, jsonStr);
    xQueueSend(g_mqttPubDataQueueHandler, &_mqttPubData, pdMS_TO_TICKS(100));
    cJSON_free(jsonStr);
    cJSON_Delete(msgBuff);
}

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
        else if (mqttCmdType == START_OTA_FROM_MQTT_CMD_PARAMETER)
        {
            cJSON *urlJson = getJSONobj(data, "url");
            cJSON *fileNameJson = getJSONobj(data, "file_name");
            if (urlJson == NULL || fileNameJson == NULL)
            {
                return ESP_FAIL;
            }
            char *urlStrBuf = cJSON_GetStringValue(urlJson);
            if (urlStrBuf == NULL)
            {
                ESP_LOGE(TAG, "OTA URL is NULL");
                return ESP_FAIL;
            }
            char *fileNameStrBuf = cJSON_GetStringValue(fileNameJson);
            if (fileNameStrBuf == NULL)
            {
                ESP_LOGE(TAG, "OTA File name is NULL");
                return ESP_FAIL;
            }
            strcpy(g_nvsData.networkConfigData.otaConfigData.esp32OtaServer, urlStrBuf);
            strcpy(g_nvsData.networkConfigData.otaConfigData.firmwareFileName, fileNameStrBuf);
            ESP_ERROR_CHECK(saveConfigToNvs(&g_nvsData));
            xSemaphoreGive(g_startOtaTaskSemphHandle);
        }
        else if (mqttCmdType == GET_OTA_NVS_PARAMETER) // 查询NVS存储的OTA参数
        {
            mqttPubOtaParameterMsg();
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
