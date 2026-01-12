/**
 * @file user_tasks.h
 * @brief 用户RTOS任务头文件
 * @version 1.0
 * @date 2024-01-04
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _USER_TASKS_H_
#define _USER_TASKS_H_

#include "common.h"

// RTOS TASK
#define MQTT_TASK_PRIVILEGE                             12
#define LEDSTRIP_INDICATION_TASK_PRIVILEGE              11
#define SCREEN_TASK_PRIVILEGE                           11
#define MODBUS_TASK_PRIVILEGE                           10
#define OTA_TASK_PRIVILEGE                              1
#define NETWORK_TASK_PRIVILEGE                          1

extern QueueHandle_t g_mqttRecvDataQueueHandler;    // MQTT 数据接收队列
extern QueueHandle_t g_mqttPubDataQueueHandler;     // MQTT 数据发送队列
extern QueueHandle_t g_dioInpDataQueueHandler;      // DIO 输入数据接收队列

extern void screenCmdRecvTask(void *pvParameters);
extern void networkTask(void *pvParameters);
extern void mqttTask(void *pvParameters);
extern void modbusTask(void *pvParameters);
extern void ledStripIndicationTask(void *pvParameters);
extern void otaTask(void *pvParameters);

#endif // _USER_TASKS_H_
