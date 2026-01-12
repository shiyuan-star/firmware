/**
 * @file ota.h
 * @brief ota头文件
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _OTA_H_
#define _OTA_H_

#include "common.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#define OTA_SERVER_RECV_TIMEOUT_MS 3000 // OTA服务器尝试超时时间
extern SemaphoreHandle_t g_startOtaTaskSemphHandle;

#endif // _OTA_H_
