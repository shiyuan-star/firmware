/**
 * @file network.h
 * @brief 网络状态头文件
 * @version 1.0
 * @date 2024-01-18
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "common.h"
#include "mqtt.h"

#define NETWORT_STATE_UPDATE(x)                                         \
    do                                                                  \
    {                                                                   \
        if (xSemaphoreTake(g_sysStateInfoMetex, portMAX_DELAY) == pdTRUE) \
        {                                                               \
            g_sysStateInfo.network = x;                                   \
            xSemaphoreGive(g_sysStateInfoMetex);                          \
        }                                                               \
    } while (0)

typedef enum
{
    NET_TASK_INIT = 0x00,
    NET_TASK_SNTP_INIT,
    NET_TASK_SNTP_CHECK,
    NET_TASK_SNTP_READY,
    NET_TASK_MQTT_INIT,
    NET_TASK_MQTT_CHECK,
    NET_TASK_MQTT_READY,
    NET_TASK_READY,
    NET_TASK_QUIT
} NetTaskState_t;

extern NetTaskState_t getNetTaskState();
extern void switchNetTaskState(NetTaskState_t netTaskState);
extern void sntpTaskInit();
extern void showSystemTime();
extern int checkIsIpv4(char *domain);
extern void networkTaskLoop();

#endif // _NETWORK_H_
