/**
 * @file config.h
 * @brief cjsonx 相关头文件
 * @version 1.0
 * @date 2024-01-03
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _CONFIG_H_
#define _CONFIG_H_

// cJSONx
#include "cJSONx.h"
// Custom
#include "data_type.h"

extern const cjsonx_reflect_t ScreenState_reflection[];
extern const cjsonx_reflect_t EthConfigData_reflection[];
extern const cjsonx_reflect_t WifiConfigData_reflection[];
extern const cjsonx_reflect_t OtaConfigData_reflection[];
extern const cjsonx_reflect_t NtpConfigData_reflection[];
extern const cjsonx_reflect_t MqttConfigData_reflection[];
extern const cjsonx_reflect_t OtaConfigData_reflection[];
extern const cjsonx_reflect_t NetworkConfigData_reflection[];
extern const cjsonx_reflect_t LedstripConfigData_reflection[];
extern const cjsonx_reflect_t DeviceConfigData_reflection[];
extern const cjsonx_reflect_t LedStripIndicationConfigData_reflection[];
extern const cjsonx_reflect_t ProjectConfigData_reflection[];
extern const cjsonx_reflect_t NvsData_reflection[];
extern const cjsonx_reflect_t SysStateInfo_reflection[];

#endif //_CONFIG_H_