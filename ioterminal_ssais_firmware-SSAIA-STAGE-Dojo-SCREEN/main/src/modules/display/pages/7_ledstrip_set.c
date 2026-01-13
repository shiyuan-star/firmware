/**
 * @file ledstrip_set.c
 * @brief 灯带设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  灯带设置页面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t ledstripSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static LedstripConfigData_t ledstripConfigData;
    if (!initialized)
    {
        ledstripConfigData = g_nvsData.DeviceConfigData.ledstripConfigData;
        initialized = true;
    }
    switch (controlId)
    {
    case SCREEN_LEDSTRIP_ENABLE_BUTTON:
        ledstripConfigData.ledstripEnabled = param[1];
        break;

    case SCREEN_LEDSTRIP_LED_NUM_TEXT:
        ledstripConfigData.ledNum = str2num((char *)param);
        break;

    case SCREEN_LEDSTRIP_LED_MODLE_TEXT:
        ledstripConfigData.ledModel = str2num((char *)param);
        break;

    case SCREEN_LEDSTRIP_LED_MODLE_INFO_BUTTON:
        SetTextValue(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, (uint8_t *)"0:WS2812\n1:SK6812\n2:WS2815B\n3:WS2815F");
        break;

    case SCREEN_LEDSTRIP_PIXEL_FORMAT_TEXT:
        ledstripConfigData.pixelFormat = str2num((char *)param);
        break;

    case SCREEN_LEDSTRIP_PIXEL_FORMAT_INFO_BUTTON:
        SetTextValue(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, (uint8_t *)"0:GRB\n1:RGB");
        break;

    case SCREEN_LEDSTRIP_BRIGHTNESS_TEXT:
        ledstripConfigData.btightness = str2num((char *)param);
        break;

    case SCREEN_LEDSTRIP_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (configStructCmp(&ledstripConfigData, &(g_nvsData.DeviceConfigData.ledstripConfigData), LedstripConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.DeviceConfigData.ledstripConfigData = ledstripConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功，重启后生效.", "Successfully saved, takes effect after restarting.", "保存に成功し、再起動後に有効になります。");
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
