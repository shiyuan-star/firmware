/**
 * @file common.h
 * @brief 通用模块头文件
 * @version 1.0
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__

// C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
// ESP32
#include "sdkconfig.h"
#include "esp_chip_info.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_app_desc.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_sntp.h"
#include "esp_https_ota.h"
// RTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include <sys/time.h>
// Network
#include <sys/socket.h>
#include "netinet/in.h"
#include "lwip/err.h"
#include "lwip/sys.h"
// Driver
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "led_strip.h"
// MQTT
#include "mqtt_client.h"
// OTA
#include "ota.h"
// Modbus
#include "mbcontroller.h"
#include "modbus.h"
// Modules
#include "screen.h"
#include "screen_uart.h"
#include "screen_queue.h"
#include "screen_config.h"
#include "screen_driver.h"
#include "gpio_peripheral.h"
#include "wireless.h"
#include "ethernet.h"
#include "network.h"
#include "ledstrip.h"
// USB
#include "usbh_core.h"
#include "usbh_hid.h"
#include "hid_host.h"
// Custom
#include "board.h"
#include "default_config.h"
#include "nvs_storage.h"
#include "config.h"
#include "user_tasks.h"
#include "data_type.h"
#include "business.h"
#include "mqtt_cmd_type.h"

// 宏定义
#define FIRMWARE_VERSION "V0.0.1" // 固件版本名
#define NETWORK_NVS_DATA_OVERWRITE true   // 更新固件用default_config覆写网络存储的配置
#define DEVICE_NVS_DATA_OVERWRITE false    // 覆写外设存储的配置
#define PROJECT_NVS_DATA_OVERWRITE true   // 覆写项目存储的配置
#define INVALID_CHECKSUM "0000000000000000"

// 全局变量
extern SemaphoreHandle_t g_screenStateMutex;       // 屏幕状态互斥信号量
extern ScreenState_t g_screenState;                // 屏幕状态
extern SemaphoreHandle_t g_sysStateInfoMetex;      // 系统状态互斥信号量
extern SysStateInfo_t g_sysStateInfo;              // 系统状态
extern SemaphoreHandle_t g_ledstripRmtHandleMetex; // LED灯带句柄互斥信号量(灯带刷新需要保持RMT资源互斥)
extern led_strip_handle_t g_ledstripRmtHandle;     // LED灯带句柄
extern NvsData_t g_nvsData;                        // NVS数据
extern NvsData_t g_defaultNvsData;

// 全局函数
extern void dumpNvsData(const char *tag, NvsData_t nvsData);
extern void dumpSysStateInfo(const char *tag, SysStateInfo_t sysStateInfo);
extern bool configStructCmp(void *struct1, void *struct2, const cjsonx_reflect_t *tbl);
extern const char *getCompiledDate();
extern int bytes2HexStr(uint8_t *bytes, int len, uint8_t *hexStr);
extern int hexStr2Bytes(uint8_t *hexStr, int len, uint8_t *bytes);
extern uint32_t str2num(char *str);
extern cJSON *getJSONobj(cJSON *data, const char *const string);
extern void rgb8882HSV(unsigned char r, unsigned char g, unsigned char b, float *hue, unsigned char *saturation, unsigned char *value);

#endif // __COMMON_H__