/**
 * @file mqtt_cmd_type.h
 * @brief mqtt外发指令定义
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _MQTT_CMD_TYPE_H_
#define _MQTT_CMD_TYPE_H_

// screen_control_type 命令类型定义
#define MQTT_CONTROL_TYPE_SCREEN_CONTROL_BUTTON 16 ///< 屏幕按钮设置
#define SET_BUTTON_VALUE 1
#define MQTT_CONTROL_TYPE_SCREEN_CONTROL_TEXT 17 ///< 屏幕文本设置
#define SET_TEXT_VAULE 1
#define SET_TEXT_COLOR 5

// device_type 命令类型定义
#define MQTT_CONTROL_TYPE_DEVICE_ALARMLED 101 ///< 三色灯设置
#define SET_ALARMLED_STATE 1                  ///< 设置三色灯状态

// system_type 命令类型定义
#define MQTT_CONTROL_TYPE_SYSTEM_REBOOT 151 ///< 复位相关
#define MQTT_SET_SYSTEM_REBOOT 1            ///< 重启系统
#define NOTIFY_SYSTEM_REBOOT 1              ///< 系统重启报告
#define MQTT_CONTROL_TYPE_SYSTEM_OTA 152    ///< OTA相关
#define START_OTA_FROM_NVS_URL 1
#define NOTIFY_OTA_STATE 1                       ///< 回复OTA状态
#define START_OTA_FROM_DATA_URL 2                ///< 从指定的URL更新,会更新当前NVS存储的URL
#define GET_OTA_NVS_URL 3                        ///< 查询NVS存储的OTA地址
#define NOTIFY_OTA_NVS_URL 3                     ///< 回复NVS存储的OTA地址
#define GET_FIRMWARE_VERSION 4                   ///< 查询固件版本
#define NOTIFY_FIRMWARE_VERSION 4                ///< 回复固件版本
#define MQTT_CONTROL_TYPE_MQTT_STATE 153         ///< MQTT状态
#define NOTIFY_MQTT_OFFLINE_RECONNECTION_COUNT 2 ///< MQTT掉线重连次数报告
#define MQTT_CONTROL_TYPE_NETWORK_STATE 154      ///< 网络状态
#define NOTIFY_NETWORK_IP_ADDR 1                 ///< 报告IP地址

// business_type 命令类型定义
#define MQTT_CONTROL_TYPE_BUSINESS_ORDER_MANAGE 212           ///< 下发灯带拣货指示订单
#define PLACE_NEW_ORDER 1                                     ///< 下发新订单
#define PLACE_NEW_ORDER_BY_NODE_RED 2                         ///< 影子系统下发新订单
#define QUERY_RESIDUES_ORDER 3                                ///< 查询残留订单
#define NOTIFY_RESIDUES_ORDER 2                               ///< 回复残留订单
#define MQTT_CONTROL_TYPE_BUSINESS_PICKUP_COMPLETED 213       ///< 下发灯带拣货完成通知
#define PICKUP_COMPLETED 1                                    ///< 拣货完成
#define MQTT_CONTROL_TYPE_BUSINESS_END_PICKUP_INSTRUCTION 214 ///< 结束指示灯带全灭
#define END_PICKUP_INSTRUCTION 1                              ///< 结束指示
#define END_PICKUP_INSTRUCTION_BY_NODE_RED 2
#define MQTT_CONTROL_TYPE_BUSINESS_LEDSTRIP_DEBUG 215 ///< 灯带调试
#define LED_LOCATE 1                                  ///< 定位灯珠
#define LED_SEQUENCE 2                                ///< 灯珠顺序跑马

#endif // _MQTT_CMD_TYPE_H_