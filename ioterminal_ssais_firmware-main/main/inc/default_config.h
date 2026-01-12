/**
 * @file default_config.h
 * @brief  默认配置文件
 * @version 1.0
 * @date 2023-12-24
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#ifndef _DEFAULT_CONFIG_H_
#define _DEFAULT_CONFIG_H_

// 宏定义

// ETH
#define DEAFULT_ETH_ENABLE false
#define DEFAULT_ETH_STATIC_IP "192.168.88.123"
#define DEFAULT_ETH_NET_MASK "255.255.255.0"
#define DEFAULT_ETH_GATEWAY "192.168.88.1"
#define DEAFULT_ETH_DHCP_ENABLE true

// WiFi
#define DEAFULT_WIFI_ENABLE true
#define DEFAULT_WIFI_MAXIMUM_RETRY 9999 // WiFi尝试连接次数

#define DEFAULT_WIFI_SSID "Robot"
#define DEFAULT_WIFI_PASS "Robot2018"

// MQTT
#define DEAFULT_MQTT_ENABLE true
#define DEFAULT_MQTT_MAXIMUM_RETRY 9999 // MQTT尝试连接次数
#define DEFAULT_MQTT_LAST_WILL_MSG "{\"control_type\":153,\"notify_type\":1,\"data\":{}}" // 遗嘱消息内容

#define DEFAULT_MQTT_URL "mqtt://192.168.88.108:1883"
#define DEFAULT_MQTT_USER_NAME "IoTerminal_Client"
#define DEFAULT_MQTT_USER_PASSWORD "IoTerminal_Client_Pass"
#define DEFAULT_MQTT_SUBSCRIBE_TOPIC "Debug/IoTerminal_S3/ssais_s2c/ymslx/"
#define DEFAULT_MQTT_SUBSCRIBE_TOPIC_QOS 2
#define DEFAULT_MQTT_PUBLISH_TOPIC "Debug/IoTerminal_S3/ssais_c2s/ymslx/"
#define DEFAULT_MQTT_PUBLISH_TOPIC_QOS 2

// SNTP
#define DEAFULT_NTP_ENABLE false
#define DEFAULT_NTP_SERVER_1_URL "ntp.aliyun.com"
#define DEFAULT_NTP_SERVER_2_URL "ntp.ntsc.ac.cn"
#define DEFAULT_NTP_SERVER_3_URL "pool.ntp.org"
#define DEFAULT_TIME_ZONE "CST-8"
#define DEFAULT_ACCEPT_NO_EXTERNAL_TIME_SERVICE 0x01

// OTA
#define DEAFULT_OTA_ENABLE true
#define DEFAULT_ESP32_OTA_SERVER_1_URL "http://192.168.88.108:1880/ss-ais/ota"
#define DEFAULT_ESP32_OTA_SERVER_2_URL ""
#define DEFAULT_ESP32_OTA_SERVER_3_URL ""

// LED STRIP
#define DEAFULT_LED_STRIP_ENABLE true
#define DEAFULT_LED_STRIP_PIXEL_NUM 2400
#define DEAFULT_LED_STRIP_MODLE LED_MODEL_WS2815F
#define DEAFULT_LED_STRIP_BTIGHTNESS 20 // 0-255

// PROJECT
#define DEAFULT_USER_OVERWRITE_LOCATION_SET false          // 默认不允许覆盖库位
#define DEFAULT_LED_STRIP_INDICATION_MODLE 1               // 灯带指示模式
#define DEFAULT_LED_STRIP_INDICATION_OWN_LED_MAXNUM 1      // 默认单个用户占用灯珠的最大值
#define DEFAULT_LED_STRIP_INDICATION_COLOR_GREEN 65280     // 灯带绿色
#define DEFAULT_LED_STRIP_INDICATION_COLOR_YELLOW 16766720 // 灯带黄色
#define DEFAULT_LED_STRIP_INDICATION_COLOR_RED 13434880    // 灯带红色
#define DEFAULT_LED_STRIP_INDICATION_COLOR_BLUE 2186201    // 灯带蓝色


#endif //_DEFAULT_CONFIG_H_