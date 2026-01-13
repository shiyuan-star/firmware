/**
 * @file screen_set.c
 * @brief 屏幕设置界面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  屏幕设置界面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t screenSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    switch (controlId)
    {
    case SCREEN_SCREEN_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功,配置写入屏幕存储。", "Successfully saved, configuration written to screen storage.", "保存に成功し、書き込み画面ストレージを構成しました。");
        break;
    default:
        break;
    }
    return ESP_OK;
}
