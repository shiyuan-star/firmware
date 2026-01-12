/**
 * @file ssais_ledstrip_set.c
 * @brief ssais灯带设置
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  ssais灯带设置
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t ssaisLedstripHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static LedStripIndicationConfigData_t ledStripIndicationConfigData;
    if (!initialized)
    {
        ledStripIndicationConfigData = g_nvsData.projectConfigData.ledStripIndicationConfigData;
        initialized = true;
    }
    switch (controlId)
    {
    case SCREEN_ALLOW_USER_OVERWRITE_LOCATION_DATA_BUTTON:
        ledStripIndicationConfigData.allowOrderOverwriteLocation = param[1];
        break;
    case LED_DISTRIBUTION_MODE_TEXT:
        ledStripIndicationConfigData.indicationModle = str2num((char *)param);
        break;
    case LED_DISTRIBUTION_MODE_INFO_BUTTON:
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "1:先到先得,不限制用户占用灯珠数量,整个库位亮满\n2:限制用户最大占用灯珠数量",
                                 "1: First come, first served, no limit on the number of light beads occupied by users, the entire storage location is lit up. \n2: Limit the maximum number of light beads occupied by users",
                                 "1：ユーザーが占有するビーズの数を制限せずに、先着順で入手し、ライブラリ全体が点灯します \n2：ユーザーが占有する最大ビーズの数を制限します");
        break;
    case USER_MAXIMUM_OCCUPIED_LEDS_TEXT:
        ledStripIndicationConfigData.orderOwnLedMaxNum = str2num((char *)param);
        break;
    case GREEN_ALARM_LED_ASSOCIATED_RGB_TEXT:
        ledStripIndicationConfigData.colorGreen = str2num((char *)param);
        break;
    case YELLOW_ALARM_LED_ASSOCIATED_RGB_TEXT:
        ledStripIndicationConfigData.colorYellow = str2num((char *)param);
        break;
    case RED_ALARM_LED_ASSOCIATED_RGB_TEXT:
        ledStripIndicationConfigData.colorRed = str2num((char *)param);
        break;
    case BLUE_ALARM_LED_ASSOCIATED_RGB_TEXT:
        ledStripIndicationConfigData.colorBlue = str2num((char *)param);
        break;
    case SCREEN_PROJECT_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&ledStripIndicationConfigData, &(g_nvsData.projectConfigData.ledStripIndicationConfigData), LedStripIndicationConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.projectConfigData.ledStripIndicationConfigData = ledStripIndicationConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
