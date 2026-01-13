/**
 * @file data_type.h
 * @brief 数据类型定义头文件
 * @version 1.0
 * @date 2023-12-18
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#ifndef _DATA_TYPE_H_
#define _DATA_TYPE_H_

#include <stdio.h>

#define MAX_MAC_BUF_LEN 18
#define MAX_IP_BUF_LEN 32
#define MAX_VERSION_BUF_LEN 32
#define MAX_SSID_BUF_LEN 64
#define MAX_HASH_BUF_LEN 32
#define MAX_USERNAME_BUF_LEN 64
#define MAX_PASSWD_BUF_LEN 64
#define MAX_TIMEZONE_BUF_LEN 64
#define MAX_URL_BUF_LEN 256
#define MAX_FIRMWARE_FILE_NAME_LEN 128
#define MAX_CONFIG_LEN 2048
#define MAX_BOX_PARAM_LEN 16384
// MQTT CONFIG
#define MQTT_RECEIVE_QUEUE_LEN 8 // Mqtt 接收数据队列长度
#define MQTT_PUBISH_QUEUE_LEN 8  // Mqtt 发送数据队列长度
#define MQTT_TOPIC_MAX_LEN 128
#define MQTT_RECEIVE_DATA_MAX_LEN 16384
#define MQTT_PUBLISH_DATA_MAX_LEN 16384

// SCANNER CONFIG
#define SCANNER_INPUT_QUEUE_LEN 16      // 扫码枪输入队列长度
#define SCANNER_INPUT_DATA_MAX_SIZE 256 // 扫码枪最大接受文本长度

typedef enum
{
    ALARM_STATE_BRYG_0000 = 0x00,
    ALARM_STATE_BRYG_0001,
    ALARM_STATE_BRYG_0010,
    ALARM_STATE_BRYG_0011,
    ALARM_STATE_BRYG_0100,
    ALARM_STATE_BRYG_0101,
    ALARM_STATE_BRYG_0110,
    ALARM_STATE_BRYG_0111,
    ALARM_STATE_BRYG_1000,
    ALARM_STATE_BRYG_1001,
    ALARM_STATE_BRYG_1010,
    ALARM_STATE_BRYG_1011,
    ALARM_STATE_BRYG_1100,
    ALARM_STATE_BRYG_1101,
    ALARM_STATE_BRYG_1110,
    ALARM_STATE_BRYG_1111,
    ALARM_STATE_NET_DISCONNECT, // 网络断开
    ALARM_STATE_SYSTEAM_STAND_BY, // 系统就绪
    ALARM_STATE_INDICATION_ERR, // 订单数量超过库位灯珠数量，无法指示报警
    ALARM_STATE_MAX,
} AlarmLedState;

typedef enum
{
    SCREEN_CHINESE = 0x00,
    SCREEN_ENGLISH,
    SCREEN_JAPANESE,
} ScreenLanguage;

typedef struct _ScreenState
{
    bool connectState;
    uint16_t lastScreenId; // 上一个ID页面
    uint16_t screenId;     // 当前ID页面
    uint8_t backLight;
    uint8_t language;
    uint8_t waitCheckEvent; // 等待对话框确认的事件
} ScreenState_t;

typedef struct _MqttReceiveData
{
    uint8_t topicLen;
    char topic[MQTT_TOPIC_MAX_LEN];
    uint16_t dataLen;
    char data[MQTT_RECEIVE_DATA_MAX_LEN];
} MqttReceiveData_t;

typedef struct _MqttPublishData
{
    uint16_t dataLen;
    char data[MQTT_PUBLISH_DATA_MAX_LEN];
} MqttPublishData_t;

typedef struct _DioInputData
{
    uint8_t inputPort; // 输入口
    uint8_t vaule;     // 当前数值
    uint32_t time;     // 接收时间戳
} DioInputData_t;

typedef struct _ScannerInputData
{
    uint16_t dataLen;
    char data[SCANNER_INPUT_DATA_MAX_SIZE];
} ScannerInputData_t;

typedef struct _EthConfigData
{
    bool ethernetEnabled;
    char staticIp[MAX_IP_BUF_LEN];
    char netMask[MAX_IP_BUF_LEN];
    char gateway[MAX_IP_BUF_LEN];
    bool ethDhcpEnabled;
} EthConfigData_t;

typedef struct _WifiConfigData
{
    bool wifiEnabled;
    char ssid[MAX_SSID_BUF_LEN];
    char password[MAX_PASSWD_BUF_LEN];
    uint8_t maximumRetry;
} WifiConfigData_t;

typedef struct _MqttConfigData
{
    bool mqttEnabled;
    char url[MAX_URL_BUF_LEN];
    char userName[MAX_USERNAME_BUF_LEN];
    char userPassword[MAX_PASSWD_BUF_LEN];
    uint8_t maximumRetry;
    char subTopic[MQTT_TOPIC_MAX_LEN];
    uint8_t subQos;
    char pubTopic[MQTT_TOPIC_MAX_LEN];
    uint8_t pubQos;
    // char lastWillTopic[MQTT_TOPIC_MAX_LEN];
} MqttConfigData_t;
typedef struct _NtpConfigData
{
    bool ntpEnabled;
    char ntpServer1[MAX_URL_BUF_LEN];
    char ntpServer2[MAX_URL_BUF_LEN];
    char ntpServer3[MAX_URL_BUF_LEN];
    char timeZone[MAX_TIMEZONE_BUF_LEN];
    bool ntpAccepNoExternalTimeService;
} NtpConfigData_t;

typedef struct _otaConfigData
{
    bool otaEnabled;
    char esp32OtaServer[MAX_URL_BUF_LEN];
    char firmwareFileName[MAX_FIRMWARE_FILE_NAME_LEN];
} OtaConfigData_t;

typedef struct _NetworkConfigData
{
    EthConfigData_t ethConfigData;
    WifiConfigData_t wifiConfigData;
    NtpConfigData_t ntpConfigData;
    MqttConfigData_t mqttConfigData;
    OtaConfigData_t otaConfigData;
} NetworkConfigData_t;

typedef struct _LedstripConfigData
{
    bool ledstripEnabled;
    uint16_t ledNum;
    uint8_t ledModel;
    uint8_t pixelFormat;
    uint8_t btightness;
} LedstripConfigData_t;

typedef struct _RS485ConfigData_t
{
    bool rs485Enabled;
    uint32_t baudrate;  // 通信波特率
    uint8_t slaveNum;   // 从机设备数量
    uint8_t slaveModel; // 从机设备通信模式
} RS485ConfigData_t;

typedef struct _DeviceConfigData
{
    LedstripConfigData_t ledstripConfigData;
    RS485ConfigData_t rs485ConfigData;
} DeviceConfigData_t;

typedef struct _LedStripIndicationConfigData_t
{
    bool allowOrderOverwriteLocation;
    uint8_t indicationModle;    // 灯带指示模式
    uint16_t orderOwnLedMaxNum; // 单个订单占用灯珠的最大值
    uint32_t colorGreen;        // 灯带指示绿色
    uint32_t colorYellow;       // 灯带指示黄色
    uint32_t colorRed;          // 灯带指示红色
    uint32_t colorBlue;         // 灯带指示蓝色
} LedStripIndicationConfigData_t;

typedef struct _ProjectConfigData
{
    LedStripIndicationConfigData_t ledStripIndicationConfigData;
} ProjectConfigData_t;
typedef struct _NvsData
{
    NetworkConfigData_t networkConfigData;
    DeviceConfigData_t DeviceConfigData;
    ProjectConfigData_t projectConfigData;
    char version[MAX_VERSION_BUF_LEN];
    char checksum[MAX_HASH_BUF_LEN]; // sha256 256bit 32字节
} NvsData_t;

typedef enum
{
    NET_DISCONNECT = 0x00,
    ETH_CONNECT,
    WIFI_CONNECT
} NetworkState_t;

typedef struct _SysStateInfo
{
    char staMac[MAX_MAC_BUF_LEN];       // MAC地址字符串
    char ipAddr[MAX_IP_BUF_LEN];        // IP地址字符串
    uint32_t bootFreeHeapSizeInternal;  // 系统启动后的片上堆内存
    uint32_t bootFreeHeapSizePsram;     // 系统启动后的外部PSRAM内存
    NetworkState_t network;             // 网络状态
    uint16_t mqttOfflineReconnectCount; // MQTT掉线重连计数
} SysStateInfo_t;

#endif //_DATA_TYPE_H_