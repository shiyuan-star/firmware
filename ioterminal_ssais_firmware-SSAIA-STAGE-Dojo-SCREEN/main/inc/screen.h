/**
 * @file screen.h
 * @brief 屏幕驱动头文件
 * @version 1.0
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "common.h"
#include "screen_config.h"

#define SCREEN_POWER_ON gpio_set_level(CONFIG_SCREEN_POWER_ENABLE_PIN, CONFIG_IS_POWER_ON_WHEN_LEVEL_HIGH);
#define SCREEN_POWER_OFF gpio_set_level(CONFIG_SCREEN_POWER_ENABLE_PIN, !CONFIG_IS_POWER_ON_WHEN_LEVEL_HIGH);

#define SCREEN_ID_UPDATE(x)                                              \
    do                                                                   \
    {                                                                    \
        if (xSemaphoreTake(g_screenStateMutex, portMAX_DELAY) == pdTRUE) \
        {                                                                \
            if (g_screenState.screenId == x)                             \
            {                                                            \
            }                                                            \
            else                                                         \
            {                                                            \
                g_screenState.lastScreenId = g_screenState.screenId;     \
                g_screenState.screenId = x;                              \
            }                                                            \
            xSemaphoreGive(g_screenStateMutex);                          \
        }                                                                \
    } while (0)

/**
 * @brief  需要对话框确认的屏幕事件
 */
typedef enum
{
    REBOOT_CHECK = 0x01,
    SHUTDOWN_CHECK,
    DELETE_ALL_BOX_CHECK, // 删除所有库位确认
    SCREEN_CHECK_EVENT_MAX,
} ScreenCheckEvent;

extern esp_err_t screenCmdRecvHandle(PCTRL_MSG msg, uint16_t size);
extern void screenInfoUpdate(uint16_t screenId);
extern void setScreenPage(uint16_t screen_id);
extern void setTextValueMultilingual(uint16_t screen_id, uint16_t control_id, char *chineseStr, char *englishStr, char *japaneseStr);

typedef struct
{
    uint16_t screenId;
    esp_err_t (*screen_page_handle)(uint16_t controlId, uint8_t param[256], uint16_t size);
} screen_page_handle_t;

extern esp_err_t systemInfoSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t networkSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t protocolSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t mqttSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t ntpSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t otaSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t deviceSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t ledstripSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t ledstripDebugHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t rs485SetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t dioSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t screenSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t projectSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t ssaisProjectHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t messageDialogHandle(uint16_t controlId, uint8_t param[256], uint16_t size);
extern esp_err_t checkDialogHandle(uint16_t controlId, uint8_t param[256], uint16_t size);

#endif //_SCREEN_H_