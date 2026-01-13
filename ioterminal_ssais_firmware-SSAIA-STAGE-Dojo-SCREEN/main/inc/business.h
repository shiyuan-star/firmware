/**
 * @file business.h
 * @brief 业务相关头文件
 * @version 1.0
 * @date 2024-04-15
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _BUSINESS_H_
#define _BUSINESS_H_

// Custom
#include "data_type.h"

#define LED_STRIP_INDICATION_MAX_ORDER_BOX_SIZE 256          ///< 灯带亮灯指示一次订单下发能承载的最大物料
#define LED_STRIP_INDICATION_ORDER_STR_MAXSIZE 48            ///< 订单字符串最大值
#define LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE 32 ///< 灯带库位字符串最大值

// 定义事件标志
#define TOUCH_CENTER_MODE_BIT (1 << 0)  // 检测拿取位置中心模式
#define TOUCH_MIN_MAX_MODE_BIT (1 << 1) // 检测物料盒大小模式

typedef struct _BoxParam
{
    uint16_t minX;
    uint16_t minY;
    uint16_t maxX;
    uint16_t maxY;
    uint16_t beginLed;
    uint16_t endLed;
    char boxName[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE];
} BoxParam_t;

typedef enum
{
    LED_STRIP_RED = 0x00,
    LED_STRIP_GREEN,
    LED_STRIP_BLUE,
    LED_STRIP_YELLOW,
} LedStripColor;

// 触摸点数据缓存
typedef struct
{
    uint16_t x;
    uint16_t y;
} TouchPoint_t;

// 库位点数计算结构体
typedef struct
{
    char boxName[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE];
    uint16_t pointCount;
} BoxPointCount_t;

/**
 * @brief 订单信息
 */
typedef struct g_orderInfo
{
    char orderName[LED_STRIP_INDICATION_ORDER_STR_MAXSIZE]; // 订单名称
    uint16_t orderCount;                                    // 订单重复次数
    uint64_t timeStamp;                                     // 订单时间
} OrderInfo_t;

extern QueueHandle_t g_ledStripBoxDataQueueHandler;
extern SemaphoreHandle_t g_ledStripNewOrderSemphHandle;
extern SemaphoreHandle_t g_ledStripCancelOrderSemphHandle;
extern OrderInfo_t g_orderInfo;
extern esp_err_t queryResiduesOrder();
extern esp_err_t ledStripKillAllOrder();
extern EventGroupHandle_t g_ifTouchDataFLowEventGroup; // 触摸传感器数据流信号量
extern BoxParam_t g_drawBoxParam;                      // 灯带库位参数

#endif //_BUSINESS_H_