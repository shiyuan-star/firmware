/**
 * @file message_dialog.c
 * @brief 消息对话框处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  对话框处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t messageDialogHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_MESSAGE_DAILOG_YES_BUTTON:
        setScreenPage(g_screenState.lastScreenId);
        SetTextClena(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT);
        break;
    default:
        break;
    }
    return ESP_OK;
}
