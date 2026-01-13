/**
 * @file ntp_set.c
 * @brief NTP设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  NTP设置页面处理
 * @param  controlId
 * @param  param
 * @return esp_err_t
 */
esp_err_t ntpSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static NtpConfigData_t ntpConfigData;
    if (!initialized)
    {
        ntpConfigData = g_nvsData.networkConfigData.ntpConfigData;
        initialized = true;
    }

    // 根据控件ID，修改相应的配置数据

    switch (controlId)
    {
    case SCREEN_NTP_ENABLE_BUTTON:
        ntpConfigData.ntpEnabled = param[1];
        break;
    case SCREEN_PROJECT_NTP_SERVER_1_URL_TEXT:
        strcpy(ntpConfigData.ntpServer1, (char *)param);
        break;
    case SCREEN_PROJECT_NTP_SERVER_2_URL_TEXT:
        strcpy(ntpConfigData.ntpServer2, (char *)param);
        break;
    case SCREEN_PROJECT_NTP_SERVER_3_URL_TEXT:
        strcpy(ntpConfigData.ntpServer3, (char *)param);
        break;
    case SCREEN_PROJECT_TIME_ZONE_TEXT:
        strcpy(ntpConfigData.timeZone, (char *)param);
        break;
    case SCREEN_NTP_ACCEPT_NO_EXTERNAL_TIME_SERVICE_BUTTON:
        ntpConfigData.ntpAccepNoExternalTimeService = param[1];
        break;
    case SCREEN_NTP_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&ntpConfigData, &(g_nvsData.networkConfigData.ntpConfigData), NtpConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.networkConfigData.ntpConfigData = ntpConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
