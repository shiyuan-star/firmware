/**
 * @file nvs_storage.h
 * @brief NVS存储头文件
 * @version 1.0
 * @date 2024-01-18
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */

#ifndef _NVS_CONFIG_H_
#define _NVS_CONFIG_H_

#include "nvs.h"
#include "data_type.h"

// 宏定义
#define NVS_NAMESPACE               "storage"
#define NVS_KEY_ELF_SHA256_VAULE    "esv"   // 记录elf文件的SHA256值，用来对比当前固件信息 判断NVS的数据是否使用默认值
#define NVS_KEY_APP_VERSION         "app_ver"
#define NVS_KEY_CONFIG_DATA         "cfg_data"

extern nvs_handle_t nvsInit(void);
extern esp_err_t saveConfigToNvs(NvsData_t *nvsData);
extern esp_err_t readConfigFromNvs(NvsData_t *nvsData);
extern esp_err_t readNvsDataConfig(NvsData_t *nvsData);

#endif //_NVS_CONFIG_H_