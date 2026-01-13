/**
 * @file mqtt.h
 * @brief mqtt 头文件
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _MQTT_H_
#define _MQTT_H_

#include "common.h"
#include "cJSON.h"

// MQTT 状态定义
typedef enum
{
    MQTT_DISCONNECT = 0x00,
    MQTT_CONNECT,
    MQTT_SUBSCRIBE,
    MQTT_READY,
    MQTT_QUIT,
} MqttState_t;

// MQTT 命令大类型分类
typedef enum
{
    MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_CONTROL = 0x00,
    MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_STATE,
    MQTT_CONTROL_TYPE_CLASSIFY_DEVICE,
    MQTT_CONTROL_TYPE_CLASSIFY_SYSTEM,
    MQTT_CONTROL_TYPE_CLASSIFY_BUSINESS,
    MQTT_CONTROL_TYPE_CLASSIFY_MAX,
} MqttControlTypeClassify_t;

// MQTT 命令类型范围
#define MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SCREEN_CONTROL 1
#define MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SCREEN_CONTROL 49
#define MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SCREEN_STATE 50
#define MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SCREEN_STATE 99
#define MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_DEVICES 100
#define MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_DEVICES 149
#define MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SYSTEAM 150
#define MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SYSTEAM 199
#define MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_BUSINESS 200
#define MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_BUSINESS 249

// MQTT命令类型范围定义与处理结构体
typedef struct
{
    uint8_t mqttControlTypeClassify; // control_type分类
    uint16_t minClassifyNum;         // 最小的control_type值
    uint16_t maxClassifyNum;         // 最大的control_type值
    esp_err_t (*mqtt_cmd_handle)(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);
} mqtt_cmd_handle_t;

extern esp_mqtt_client_handle_t g_mqttClientHandle;

extern esp_err_t mqttSetScreenControlHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);
extern esp_err_t mqttSetScreenStateHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);
extern esp_err_t mqttSetDeviceHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);
extern esp_err_t mqttSetSystemHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);
extern esp_err_t mqttSetBusinessHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data);

extern MqttState_t getMqttState();
extern void switchMqttState(MqttState_t mqttState);
extern esp_mqtt_client_handle_t mqttInit(MqttConfigData_t *mqttConfigData);
extern void mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
extern void mqttDefaultTopicPubStrMsg(uint16_t controlType, uint16_t notifyType, const char *jsonObjName, const char *str);
extern void mqttDefaultTopicPubNumMsg(uint16_t controlType, uint16_t notifyType, const char *jsonObjName, uint32_t num);

#endif // _MQTT_H_