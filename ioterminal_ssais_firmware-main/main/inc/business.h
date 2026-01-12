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
#define LED_STRIP_INDICATION_MAX_ORDERS 4                    ///< 灯带多订单指示 最大支持订单数量
#define LED_STRIP_INDICATION_ORDER_STR_MAXSIZE 32            ///< 订单字符串最大值
#define LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE 32 ///< 灯带库位字符串最大值
#define LED_STRIP_INDICATION_BOX_LIST_ITEM_SIZE 4            ///< 灯带库位数据负载的长度  [ 库位,起始灯珠,结尾灯珠,灭灯寿命 ]

#define LED_STRIP_INDICATION_LED_LOCATE_TIMEOUT 10000         ///< 灯珠定位的时候显示残留的时间 （毫秒）

/**
 * @brief 订单对库位的占用信息
 */
typedef struct _orderBoxInfo
{
    uint16_t orderNo;         // 订单到达顺序
    uint16_t takeTimes;       // 库位取物次数
    uint16_t ownerStartLedId; // 订单占用的起始灯珠
    uint16_t ownerEndLedId;   // 订单占用的起始灯珠
    uint32_t color;           // 指示颜色
} OrderBoxInfo_t;

/**
 * @brief 库位的基础数据
 */
typedef struct _BoxData
{
    char storageLocation[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE];
    uint16_t startLedId;
    uint16_t endLedId;
    bool isBoxOrderChanged;                                       // 用户数据是否发生变化
    OrderBoxInfo_t orderBoxInfo[LED_STRIP_INDICATION_MAX_ORDERS]; // 库位的订单数据
} BoxData_t;

extern QueueHandle_t g_ledStripBoxDataQueueHandler;
extern SemaphoreHandle_t g_ledStripBoxDataSemphHandle;

#endif //_BUSINESS_H_