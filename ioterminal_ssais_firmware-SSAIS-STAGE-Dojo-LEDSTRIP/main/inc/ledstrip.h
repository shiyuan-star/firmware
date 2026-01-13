/**
 * @file ledstrip.h
 * @brief ledstrip头文件
 * @version 1.0
 * @date 2024-02-05
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _LEDSTRIP_H_
#define _LEDSTRIP_H_

#include "common.h"

#define LEDSTRIP_REFRESH                                                       \
    do                                                                         \
    {                                                                          \
        if (xSemaphoreTake(g_ledstripRmtHandleMetex, portMAX_DELAY) == pdTRUE) \
        {                                                                      \
            led_strip_refresh(g_ledstripRmtHandle);                            \
            xSemaphoreGive(g_ledstripRmtHandleMetex);                          \
        }                                                                      \
    } while (0)

#define LEDSTRIP_CLEAR                                                         \
    do                                                                         \
    {                                                                          \
        if (xSemaphoreTake(g_ledstripRmtHandleMetex, portMAX_DELAY) == pdTRUE) \
        {                                                                      \
            led_strip_clear(g_ledstripRmtHandle);                              \
            xSemaphoreGive(g_ledstripRmtHandleMetex);                          \
        }                                                                      \
    } while (0)

extern led_strip_handle_t LedStripInit(LedstripConfigData_t ledstripConfigData);

#endif // _LEDSTRIP_H_