/**
 * @file hid_host.h
 * @brief hid host 头文件
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#ifndef _HID_HOST_H_
#define _HID_HOST_H_

#define INFRARED_TOUCH_DATA_LENGTH 62          // 多点红外触摸数据长度
#define INFRARED_TOUCH_DATA_VALUE_MAXNUM 32767 // 红外触摸数据最大值
#define INFRARED_TOUCH_DATA_VALUE_MINNUM 0     // 红外触摸数据最小值
#define INFRARED_TOUCH_DATA_FRAME_MAX_POINTS 6 // 红外触摸数据单报文最大点数

// 扫码枪
#define HID_MAX_BUFFER 128              // HID最大报文数据长度

extern void infraredTouchReportHandel(const uint8_t *const data);
extern QueueHandle_t g_ScannerInputQueueHandler;

#endif //_HID_HOST_H_