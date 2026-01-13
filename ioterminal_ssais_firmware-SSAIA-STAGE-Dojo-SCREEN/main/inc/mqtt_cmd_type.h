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
#define START_OTA_FROM_MQTT_CMD_PARAMETER 2      ///< 从指定的参数请求OTA升级,会更新当前NVS存储的URL
#define GET_OTA_NVS_PARAMETER 3                  ///< 查询NVS存储的OTA参数
#define NOTIFY_OTA_NVS_PARAMETER 3               ///< 回复NVS存储的OTA参数
#define GET_FIRMWARE_VERSION 4                   ///< 查询固件版本
#define NOTIFY_FIRMWARE_VERSION 4                ///< 回复固件版本
#define MQTT_CONTROL_TYPE_MQTT_STATE 153         ///< MQTT状态
#define NOTIFY_MQTT_OFFLINE_RECONNECTION_COUNT 2 ///< MQTT掉线重连次数报告
#define MQTT_CONTROL_TYPE_NETWORK_STATE 154      ///< 网络状态
#define NOTIFY_NETWORK_IP_ADDR 1                 ///< 报告IP地址

// business_type 命令类型定义
#define MQTT_CONTROL_TYPE_BUSINESS_ORDER 230 ///< 指示订单相关
#define PLACE_NEW_ORDER 1                    ///< 下发新订单，库位配置存储在NVS中
#define CANCEL_ORDER 2                       ///<  结束订单
#define NOTIFY_STEP_DONE 1                   ///<  结束订单 上报步骤完成

#define MQTT_CONTROL_TYPE_BUSINESS_BOX_OPERATE 231 ///< 库位操作相关
#define QUERY_BOX_INFO 1                           ///< 查询库位信息
#define MODIFY_BOX_INFO 2                          ///< 修改库位信息
#define DELETE_BOX_INFO 3                          ///< 删除库位信息
#define DELETE_ALL_BOX_INFO 4                      ///< 删除所有库位信息
#define LIGHT_UP_BOX 5                             ///< 交互点亮库位
#define NOTIFY_BOX_INFO 1                          ///< 库位上报

#define MQTT_CONTROL_TYPE_BUSINESS_LEDSTRIP_OPERATE 232 ///< 灯带操作相关
#define LED_SEQUENCE_RUN 1                              ///< 设置跑马灯

#endif // _MQTT_CMD_TYPE_H_