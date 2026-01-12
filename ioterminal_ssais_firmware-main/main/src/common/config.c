/**
 * @file config.c
 * @brief cjsonx结构体反射定义
 * @version 1.0
 * @date 2024-01-18
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */

#include "common.h"

const cjsonx_reflect_t ScreenState_reflection[] = {
    __cjsonx_bool(ScreenState_t, connectState),
    __cjsonx_int(ScreenState_t, screenId),
    __cjsonx_int(ScreenState_t, backLight),
    __cjsonx_end()
};

const cjsonx_reflect_t OtaConfigData_reflection[] = {
    __cjsonx_bool(OtaConfigData_t, otaEnabled),
    __cjsonx_str(OtaConfigData_t, esp32OtaServer1),
    __cjsonx_str(OtaConfigData_t, esp32OtaServer2),
    __cjsonx_str(OtaConfigData_t, esp32OtaServer3),
    __cjsonx_end()
};

const cjsonx_reflect_t EthConfigData_reflection[] = {
    __cjsonx_bool(EthConfigData_t, ethernetEnabled),
    __cjsonx_str(EthConfigData_t, staticIp),
    __cjsonx_str(EthConfigData_t, netMask),
    __cjsonx_str(EthConfigData_t, gateway),
    __cjsonx_bool(EthConfigData_t, ethDhcpEnabled),
    __cjsonx_end()
};

const cjsonx_reflect_t WifiConfigData_reflection[] = {
    __cjsonx_bool(WifiConfigData_t, wifiEnabled),
    __cjsonx_str(WifiConfigData_t, ssid),
    __cjsonx_str(WifiConfigData_t, password),
    __cjsonx_int(WifiConfigData_t, maximumRetry),
    __cjsonx_end()
};

const cjsonx_reflect_t NtpConfigData_reflection[] = {
    __cjsonx_bool(NtpConfigData_t, ntpEnabled),
    __cjsonx_str(NtpConfigData_t, ntpServer1),
    __cjsonx_str(NtpConfigData_t, ntpServer2),
    __cjsonx_str(NtpConfigData_t, ntpServer3),
    __cjsonx_str(NtpConfigData_t, timeZone),
    __cjsonx_bool(NtpConfigData_t, ntpAccepNoExternalTimeService),
    __cjsonx_end()
};

const cjsonx_reflect_t MqttConfigData_reflection[] = {
    __cjsonx_bool(MqttConfigData_t, mqttEnabled),
    __cjsonx_str(MqttConfigData_t, url),
    __cjsonx_str(MqttConfigData_t, userName),
    __cjsonx_str(MqttConfigData_t, userPassword),
    __cjsonx_int(MqttConfigData_t, maximumRetry),
    __cjsonx_str(MqttConfigData_t, subTopic),
    __cjsonx_int(MqttConfigData_t, subQos),
    __cjsonx_str(MqttConfigData_t, pubTopic),
    __cjsonx_int(MqttConfigData_t, pubQos),
    __cjsonx_end()
};

const cjsonx_reflect_t NetworkConfigData_reflection[] = {
    __cjsonx_object_ex(NetworkConfigData_t, ethConfigData, EthConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NetworkConfigData_t, wifiConfigData, WifiConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NetworkConfigData_t, ntpConfigData, NtpConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NetworkConfigData_t, mqttConfigData, MqttConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NetworkConfigData_t, otaConfigData, OtaConfigData_reflection, __serialized(true)),
    __cjsonx_end()
};

const cjsonx_reflect_t LedstripConfigData_reflection[] = {
     __cjsonx_bool(LedstripConfigData_t, ledstripEnabled),
    __cjsonx_int(LedstripConfigData_t, ledNum),
    __cjsonx_int(LedstripConfigData_t, ledModel),
    __cjsonx_int(LedstripConfigData_t, btightness),
    __cjsonx_end()
};

const cjsonx_reflect_t DeviceConfigData_reflection[] = {
    __cjsonx_object_ex(DeviceConfigData_t, ledstripConfigData, LedstripConfigData_reflection, __serialized(true)),
    __cjsonx_end()
};

const cjsonx_reflect_t LedStripIndicationConfigData_reflection[] = {
    __cjsonx_bool(LedStripIndicationConfigData_t, allowOrderOverwriteLocation),
    __cjsonx_int(LedStripIndicationConfigData_t, indicationModle),
    __cjsonx_int(LedStripIndicationConfigData_t, orderOwnLedMaxNum),
    __cjsonx_int(LedStripIndicationConfigData_t, colorGreen),
    __cjsonx_int(LedStripIndicationConfigData_t, colorYellow),
    __cjsonx_int(LedStripIndicationConfigData_t, colorRed),
    __cjsonx_int(LedStripIndicationConfigData_t, colorBlue),
    __cjsonx_end()
};

const cjsonx_reflect_t ProjectConfigData_reflection[] = {
    __cjsonx_object_ex(ProjectConfigData_t, ledStripIndicationConfigData, LedStripIndicationConfigData_reflection, __serialized(true)),
    __cjsonx_end()
};

const cjsonx_reflect_t NvsData_reflection[] = {
    __cjsonx_object_ex(NvsData_t, networkConfigData, NetworkConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NvsData_t, DeviceConfigData, DeviceConfigData_reflection, __serialized(true)),
    __cjsonx_object_ex(NvsData_t, projectConfigData, ProjectConfigData_reflection, __serialized(true)),    
    __cjsonx_str(NvsData_t, version),
    __cjsonx_str(NvsData_t, checksum),
    __cjsonx_end()
};

