/**
 * @file wireless.h
 * @brief wifi相关头文件
 * @version 1.0
 * @date 2024-01-18
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _WIRELESS_H_
#define _WIRELESS_H_

#include "common.h"


/* 事件定义，这边只关心两件事:
 * - 成功连接上WiFi，去取IP
 * - 达到最大连接重试次数，且没连接上*/
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

// WiFi事件组状态机定义
typedef enum
{
    WIFI_MGR_INIT = 0x00,
    WIFI_MGR_NETIF,
    WIFI_MGR_EVENT_LOOP,
    WIFI_MGR_WIFI_STA,
    WIFI_MGR_LOW_INIT,
    WIFI_MGR_EVENT_HANDLER,
    WIFI_MGR_MODE,
    WIFI_MGR_CONFIG,
    WIFI_MGR_START,
    WIFI_MGR_CHECK,
    WIFI_MGR_READY,
    WIFI_MGR_QUIT
} WifiManager_t;

extern WifiManager_t getWifiManager();
extern void wifiConnMgrInit(WifiConfigData_t *wifiConfigData);
extern void switchWifiConnMgr(WifiManager_t wifiMgr);
extern void wifiConnManagerLoop();

#endif // _WIRELESS_H_
