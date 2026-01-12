/**
 * @file screen_config.h
 * @brief 屏幕控件和页面ID配置文件
 * @version 1.0
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _SCREEN_CONFIG_H
#define _SCREEN_CONFIG_H

typedef enum // 屏幕设置页面
{
    SCREEN_SYSTEMSET_AND_INFO_PAGE = 0, // 系统信息与系统设置界面
    SCREEN_NETWORK_SET_PAGE,            // 网络设置界面
    SCREEN_PROTOCOL_SET_PAGE,           // 协议设置界面
    SCREEN_MQTT_SET_PAGE,               // MQTT设置界面
    SCREEN_NTP_SET_PAGE,                // NTP设置界面
    SCREEN_OTA_SET_PAGE,                // OTA设置界面
    SCREEN_DEVICE_SET_PAGE,             // 外设设置界面
    SCREEN_LEDSTRIP_SET_PAGE,           // 灯带设置界面
    SCREEN_LEDSTRIP_DEBUG_PAGE,         // 灯带调试界面
    SCREEN_RS485_SET_PAGE,              // RS485设置界面
    SCREEN_PROJECT_SET_PAGE,            // 业务工程设置界面
    SCREEN_SCREEN_SET_PAGE,             // 屏幕设置界面
    SCREEN_SSAIS_LEDSTRIP_SET_PAGE,     // ssais灯带设置界面
    SCREEN_MESSAGE_DIALOG_PAGE,         // 消息对话框界面
    SCREEN_CHECK_DIALOG_PAGE,           // 重启对话框界面
    SCREEN_SET_PAGE_MAX,                // 系统设置页面最大值
} screenSystemSetPage_t;

// 系统信息与系统设置界面
#define SCREEN_NETWORK_SET_BUTTON 24 // 网络设置界面按钮
#define SCREEN_REBOOT_BUTTON 30      // 重启按钮
#define SCREEN_SHUTDOWN_BUTTON 31    // 关机按钮
#define SCREEN_MAC_TEXT 14           // 设备MAC文本控件
#define SCREEN_MAC_QRCODE_TEXT 28    // 设备MAC二维码文本控件
#define SCREEN_NETWORK_NAME_TEXT 15  // 网络名称文本控件
#define SCREEN_IP_TEXT 16            // 设备IP文本控件
#define SCREEN_RSSI_TEXT 17          // 信号强度文本控件
#define SCREEN_MQTT_TEXT 18          // 设备MQTT文本控件
#define SCREEN_RUNTIME_TEXT 19       // 运行时长文本控件
#define SCREEN_SYSTEM_RAM_TEXT 27    // RAM文本控件
#define SCREEN_CLOCK_TIME_TEXT 23    // 时间文本控件

// 网络设置界面
#define SCREEN_ETH_NETWORK_ENABLE_BUTTON 4
#define SCREEN_ETH_STATIC_IP_TEXT 11
#define SCREEN_ETH_NET_MASK_TEXT 15
#define SCREEN_ETH_GATEWAY_TEXT 19
#define SCREEN_ETH_DHCP_ENABLE_BUTTON 18
#define SCREEN_WIFI_NETWORK_ENABLE_BUTTON 16
#define SCREEN_WIFI_SSID_TEXT 14
#define SCREEN_WIFI_PASS_TEXT 13
#define SCREEN_NETWORK_SAVE_BUTTON 2

// 协议设置界面
#define SCREEN_MQTT_SET_BUTTON 29
#define SCREEN_NTP_SET_BUTTON 1
#define SCREEN_OTA_SET_BUTTON 2

// MQTT设置界面
#define SCREEN_MQTT_ENABLE_BUTTON 20
#define SCRENN_MQTT_SERVER_URL_TEXT 11
#define SCRENN_MQTT_USERNAME_TEXT 3
#define SCRENN_MQTT_PASS_TEXT 5
#define SCRENN_MQTT_SUBTOPIC_TEXT 9
#define SCRENN_MQTT_SUBTOPIC_QOS_TEXT 29
#define SCRENN_MQTT_PUBTOPIC_TEXT 12
#define SCRENN_MQTT_PUBTOPIC_QOS_TEXT 30
#define SCREEN_MQTT_SAVE_BUTTON 13

// NTP设置界面
#define SCREEN_NTP_ENABLE_BUTTON 20
#define SCREEN_PROJECT_NTP_SERVER_1_URL_TEXT 15
#define SCREEN_PROJECT_NTP_SERVER_2_URL_TEXT 24
#define SCREEN_PROJECT_NTP_SERVER_3_URL_TEXT 26
#define SCREEN_PROJECT_TIME_ZONE_TEXT 16
#define SCREEN_NTP_ACCEPT_NO_EXTERNAL_TIME_SERVICE_BUTTON 22
#define SCREEN_NTP_SAVE_BUTTON 2

// OTA设置界面
#define SCREEN_OTA_FIRMWARE_VERSION_TEXT 16
#define SCREEN_OTA_SERVER_1_URL_TEXT 15
#define SCREEN_OTA_SAVE_BUTTON 2

// 外设设置界面
#define SCREEN_LED_STRIP_SET_BUTTON 29
#define SCREEN_LED_DEBUG_SET_BUTTON 29
#define SCREEN_RS485_SET_BUTTON 1

// LED灯带设置界面
#define SCREEN_LEDSTRIP_ENABLE_BUTTON 1
#define SCREEN_LEDSTRIP_LED_NUM_TEXT 16
#define SCREEN_LEDSTRIP_LED_MODLE_TEXT 9
#define SCREEN_LEDSTRIP_LED_MODLE_INFO_BUTTON 10
#define SCREEN_LEDSTRIP_BRIGHTNESS_TEXT 17
#define SCREEN_LEDSTRIP_SAVE_BUTTON 2

// LED灯带调试界面
#define SCREEN_LEDSTRIP_BEGIN_LED_TEXT 3
#define SCREEN_LEDSTRIP_BEGIN_LED_TEXT_DEFAULT_DATA 1
#define SCREEN_LEDSTRIP_BEGIN_LED_UP_BUTTON 5
#define SCREEN_LEDSTRIP_BEGIN_LED_DOWN_BUTTON 7
#define SCREEN_LEDSTRIP_AUTO_SYNC_BUTTON 30
#define SCREEN_LEDSTRIP_END_LED_TEXT 4
#define SCREEN_LEDSTRIP_END_LED_TEXT_DEFAULT_DATA 2
#define SCREEN_LEDSTRIP_END_LED_UP_BUTTON 8
#define SCREEN_LEDSTRIP_END_LED_DOWN_BUTTON 9
#define SCREEN_LEDSTRIP_CLEAR_BUTTON 32
#define SCREEN_LEDSTRIP_BRIGHTNESS_PROGRESS 11
#define SCREEN_LEDSTRIP_DEBUG_BRIGHTNESS_TEXT 21
#define SCREEN_LEDSTRIP_DEBUG_BRIGHTNESS_TEXT_DEFAULT_DATA 10
#define SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_TEXT 24
#define SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_DEFAULT_DATA 2
#define SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_NUM_TEXT 25
#define SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_NUM_DEFAULT_DATA 2
#define SCREEN_LEDSTRIP_DEBUG_LED_SYNC_BUTTON 27

// 工程设置界面
#define SSAIS_LEDSTRIP_BUTTON 29 // SSAIS灯带设置按钮
#define SCREEN_SCREEN_SET_BUTTON 32

// 屏幕设置界面
#define SCREEN_SCREEN_SAVE_BUTTON 7

// SSAIS灯带设置界面
#define SCREEN_ALLOW_USER_OVERWRITE_LOCATION_DATA_BUTTON 20
#define LED_DISTRIBUTION_MODE_TEXT 5
#define LED_DISTRIBUTION_MODE_INFO_BUTTON 4
#define USER_MAXIMUM_OCCUPIED_LEDS_TEXT 3
#define GREEN_ALARM_LED_ASSOCIATED_RGB_TEXT 9
#define YELLOW_ALARM_LED_ASSOCIATED_RGB_TEXT 11
#define RED_ALARM_LED_ASSOCIATED_RGB_TEXT 13
#define BLUE_ALARM_LED_ASSOCIATED_RGB_TEXT 21
#define SCREEN_PROJECT_SAVE_BUTTON 16

// 消息对话框界面
#define SCREEN_MESSAGE_DAILOG_INFO_TEXT 3
#define SCREEN_MESSAGE_DAILOG_YES_BUTTON 4

// 重启对话框界面
#define SCREEN_CHECK_DAILOG_INFO_TEXT 8
#define SCREEN_CHECK_DAILOG_YES_BUTTON 6
#define SCREEN_CHECK_DAILOG_NO_BUTTON 7

// 通知类型
#define NOTIFY_TOUCH_PRESS 0X01       // 触摸屏按下通知
#define NOTIFY_TOUCH_RELEASE 0X03     // 触摸屏松开通知
#define NOTIFY_SCREEN_BOOT_OVER 0X07  // 开机通知
#define NOTIFY_WRITE_FLASH_OK 0X0C    // 写FLASH成功
#define NOTIFY_WRITE_FLASH_FAILD 0X0D // 写FLASH失败
#define NOTIFY_READ_FLASH_OK 0X0B     // 读FLASH成功
#define NOTIFY_READ_FLASH_FAILD 0X0F  // 读FLASH失败
#define NOTIFY_MENU 0X14              // 菜单事件通知
#define NOTIFY_TIMER 0X43             // 定时器超时通知
#define NOTIFY_CONTROL 0XB1           // 控件更新通知
#define NOTIFY_READ_RTC 0XF7          // 读取RTC时间
#define MSG_GET_CURRENT_SCREEN 0X01   // 画面ID变化通知
#define MSG_GET_DATA 0X11             // 控件数据通知
#define NOTIFY_HandShake 0X55         // 握手通知

#define NOTIFY_CUSTOM_INSTRUCTIONS 0X78 // 串口屏自定义帧指令第二位
enum customInstructionsType             // 自定义帧指令第三位
{
    NOTIFY_LANGUAGE = 0x01, // 屏幕语言通知
};

#define PTR2U16(PTR) ((((uint8_t *)(PTR))[0] << 8) | ((uint8_t *)(PTR))[1])                                                                 // 从缓冲区取16位数据
#define PTR2U32(PTR) ((((uint8_t *)(PTR))[0] << 24) | (((uint8_t *)(PTR))[1] << 16) | (((uint8_t *)(PTR))[2] << 8) | ((uint8_t *)(PTR))[3]) // 从缓冲区取32位数据

// 控件类型
enum CtrlType
{
    kCtrlUnknown = 0x0,
    kCtrlButton = 0x10, // 按钮
    kCtrlText,          // 文本
    kCtrlProgress,      // 进度条
    kCtrlSlider,        // 滑动条
    kCtrlMeter,         // 仪表
    kCtrlDropList,      // 下拉列表
    kCtrlAnimation,     // 动画
    kCtrlRTC,           // 时间显示
    kCtrlGraph,         // 曲线图控件
    kCtrlTable,         // 表格控件
    kCtrlMenu,          // 菜单控件
    kCtrlSelector,      // 选择控件
    kCtrlQRCode,        // 二维码
    kCtrlIcon,          // 图标控件
};

#pragma pack(push)
#pragma pack(1) // 按字节对齐

// 屏幕消息结构体
typedef struct
{
    uint8_t cmd_head;     // 帧头
    uint8_t cmd_type;     // 命令类型(UPDATE_CONTROL)
    uint8_t ctrl_msg;     // CtrlMsgType-指示消息的类型
    uint16_t screen_id;   // 产生消息的画面ID
    uint16_t control_id;  // 产生消息的控件ID
    uint8_t control_type; // 控件类型
    uint8_t param[256];   // 可变长度参数，最多256个字节
    uint8_t cmd_tail[4];  // 帧尾
} CTRL_MSG, *PCTRL_MSG;

#pragma pack(pop)

#endif //_SCREEN_CONFIG_H
