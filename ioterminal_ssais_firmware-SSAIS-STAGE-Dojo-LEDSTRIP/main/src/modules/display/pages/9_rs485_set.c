/**
 * @file rs485SetHandle.c
 * @brief rs485设置页面
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  rs485设置页面
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t rs485SetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static RS485ConfigData_t rs485ConfigData;
    if (!initialized)
    {
        rs485ConfigData = g_nvsData.DeviceConfigData.rs485ConfigData;
        initialized = true;
    }
    switch (controlId)
    {
    case SCREEN_RS485_SET_ENABLE_BUTTON:
        rs485ConfigData.rs485Enabled = param[1];
        break;
    case SCREEN_RS485_SET_BAUD_TEXT:
        rs485ConfigData.baudrate = str2num((char *)param);
        break;

    case SCREEN_RS485_SET_DEVICES_NUM_TEXT:
        rs485ConfigData.slaveNum = str2num((char *)param);
        break;

    case SCREEN_RS485_SET_MODLE_TEXT:
        rs485ConfigData.slaveModel = str2num((char *)param);
        break;

    case SCREEN_RS485_SET_MODLE_INFO_BUTTON:
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "0:ZS-DIO\n", "0:ZS-DIO\n", "0:ZS-DIO\n");
        break;

    case SCREEN_RS485_SET_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&rs485ConfigData, &(g_nvsData.DeviceConfigData.rs485ConfigData), rs485ConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.DeviceConfigData.rs485ConfigData = rs485ConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
