/**
 * @file system_info_set.c
 * @brief 系统设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  系统设置页面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t systemInfoSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_NETWORK_SET_BUTTON:
        screenInfoUpdate(SCREEN_NETWORK_SET_PAGE);
        break;
    case SCREEN_REBOOT_BUTTON:
        g_screenState.waitCheckEvent = REBOOT_CHECK;
        setTextValueMultilingual(SCREEN_CHECK_DIALOG_PAGE, SCREEN_CHECK_DAILOG_INFO_TEXT, "确认重启?", "Confirm restart?", "再起動を確認しますか？");
        break;
    case SCREEN_SHUTDOWN_BUTTON:
        g_screenState.waitCheckEvent = SHUTDOWN_CHECK;
        setTextValueMultilingual(SCREEN_CHECK_DIALOG_PAGE, SCREEN_CHECK_DAILOG_INFO_TEXT, "确认关机?", "Confirm shutdown?", "シャットダウンを確認しますか？");
        break;
    default:
        break;
    }
    return ESP_OK;
}
