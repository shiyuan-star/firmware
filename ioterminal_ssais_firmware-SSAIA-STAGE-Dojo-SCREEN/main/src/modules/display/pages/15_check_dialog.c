/**
 * @file 14_check_dialog.c
 * @brief 确认对话框处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "screen.h"
#include "esp_sleep.h"

static char *TAG = "DAILOG"; // main文件LOG标签
/**
 * @brief  对话框处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t checkDialogHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_CHECK_DAILOG_YES_BUTTON:
        switch (g_screenState.waitCheckEvent)
        {
        case REBOOT_CHECK:
            ESP_LOGI(TAG, "REBOOT_CHECK");
            g_screenState.waitCheckEvent = SCREEN_CHECK_EVENT_MAX;
            setScreenPage(SCREEN_SYSTEMSET_AND_INFO_PAGE);
            esp_restart();
            break;
        case SHUTDOWN_CHECK:
            ESP_LOGI(TAG, "SHUTDOWN_CHECK");
            g_screenState.waitCheckEvent = SCREEN_CHECK_EVENT_MAX;
            setScreenPage(SCREEN_SYSTEMSET_AND_INFO_PAGE);
            esp_deep_sleep_start(); // 执行深度睡眠,无法唤醒
            break;
        case DELETE_ALL_BOX_CHECK:
            ESP_LOGI(TAG, "DELETE_ALL_BOX_CHECK");
            g_screenState.waitCheckEvent = SCREEN_CHECK_EVENT_MAX;
            ESP_ERROR_CHECK(deleteAllBoxParamFromNvs());
            setScreenPage(SCREEN_SSAIS_PROJECT_SET_PAGE);
            break;
        default:
            break;
        }
        SetTextClena(SCREEN_CHECK_DIALOG_PAGE, SCREEN_CHECK_DAILOG_INFO_TEXT);
        break;

    case SCREEN_CHECK_DAILOG_NO_BUTTON:
        SetTextClena(SCREEN_CHECK_DIALOG_PAGE, SCREEN_CHECK_DAILOG_INFO_TEXT);
        switch (g_screenState.waitCheckEvent)
        {
        case REBOOT_CHECK:
            setScreenPage(SCREEN_SYSTEMSET_AND_INFO_PAGE);
            break;
        case SHUTDOWN_CHECK:
            setScreenPage(SCREEN_SYSTEMSET_AND_INFO_PAGE);
            break;
        case DELETE_ALL_BOX_CHECK:
            setScreenPage(SCREEN_SSAIS_PROJECT_SET_PAGE);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
