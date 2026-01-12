/**
 * @file screenTask.c
 * @brief 串口屏处理任务
 * @version 1.0
 * @date 2024-01-04
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "user_tasks.h"
#include "common.h"

static char *TAG = "SCREEN"; // screen文件LOG标签
/**
 * @brief  屏幕命令处理任务
 * @param  pvParameters
 */
void screenCmdRecvTask(void *pvParameters)
{
    uint8_t screenCmdBuffer[SCREEN_CMD_MAX_SIZE];
    uint16_t screenCmdSize = 0;
    bool screenInited = false;
    for (;;)
    {
        screenCmdSize = queue_find_cmd(screenCmdBuffer, SCREEN_CMD_MAX_SIZE);
        if (screenCmdSize)
        {
            // ESP_LOGI(TAG, "screenCmdSize =  %d", screenCmdSize);
            ESP_ERROR_CHECK_WITHOUT_ABORT(screenCmdRecvHandle((PCTRL_MSG)screenCmdBuffer, screenCmdSize));
        }
        // 持续等待屏幕连接状态更新
        if (xSemaphoreTake(g_screenStateMutex, pdMS_TO_TICKS(1)) == pdTRUE)
        {
            if (g_screenState.connectState && !screenInited) // 屏幕已连接
            {
                ESP_LOGI(TAG, "Screen initialization completed");
                xSemaphoreGive(g_screenStateMutex);
                screenInited = true;
            }
            if (g_screenState.connectState && g_screenState.screenId == SCREEN_SYSTEMSET_AND_INFO_PAGE)
            {
                screenInfoUpdate(SCREEN_SYSTEMSET_AND_INFO_PAGE);
                vTaskDelay(pdMS_TO_TICKS(200));
            }
            xSemaphoreGive(g_screenStateMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}