/**
 * @file ota_set.c
 * @brief OTA设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  OTA设置页面处理
 * @param  controlId
 * @param  param
 * @return esp_err_t
 */
esp_err_t otaSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static OtaConfigData_t otaConfigData;
    if (!initialized)
    {
        otaConfigData = g_nvsData.networkConfigData.otaConfigData;
        initialized = true;
    }
    switch (controlId)
    {
    case SCREEN_OTA_SERVER_1_URL_TEXT:
        strcpy(otaConfigData.esp32OtaServer1, (char *)param);
        break;

    case SCREEN_OTA_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&otaConfigData, &(g_nvsData.networkConfigData.otaConfigData), OtaConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.networkConfigData.otaConfigData = otaConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
