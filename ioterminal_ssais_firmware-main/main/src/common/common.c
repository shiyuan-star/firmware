/**
 * @file common.c
 * @brief 通用模块
 * @version 1.0
 * @date 2024-01-03
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "common.h"
static const char *TAG = "COMMON";

SemaphoreHandle_t g_screenStateMutex;       // 屏幕状态互斥信号量
ScreenState_t g_screenState = {0};          // 屏幕状态
SemaphoreHandle_t g_sysStateInfoMetex;      // 系统状态互斥信号量
SysStateInfo_t g_sysStateInfo = {0};        // 系统状态
led_strip_handle_t g_ledstripRmtHandle;     // LED灯带句柄
NvsData_t g_nvsData;
// 固件默认NVS数据赋值
NvsData_t g_defaultNvsData = {
    .checksum = INVALID_CHECKSUM,
    .version = FIRMWARE_VERSION,
    .networkConfigData = {
        .ethConfigData = {
            .ethernetEnabled = DEAFULT_ETH_ENABLE,
            .staticIp = DEFAULT_ETH_STATIC_IP,
            .netMask = DEFAULT_ETH_NET_MASK,
            .gateway = DEFAULT_ETH_GATEWAY,
            .ethDhcpEnabled = DEAFULT_ETH_DHCP_ENABLE},
        .wifiConfigData = {
            .wifiEnabled = DEAFULT_WIFI_ENABLE, 
            .ssid = DEFAULT_WIFI_SSID, 
            .password = DEFAULT_WIFI_PASS, 
            .maximumRetry = DEFAULT_WIFI_MAXIMUM_RETRY},
        .mqttConfigData = {
            .mqttEnabled = DEAFULT_MQTT_ENABLE, 
            .url = DEFAULT_MQTT_URL, 
            .userName = DEFAULT_MQTT_USER_NAME, 
            .userPassword = DEFAULT_MQTT_USER_PASSWORD, 
            .maximumRetry = DEFAULT_MQTT_MAXIMUM_RETRY, 
            .subTopic = DEFAULT_MQTT_SUBSCRIBE_TOPIC, 
            .subQos = DEFAULT_MQTT_SUBSCRIBE_TOPIC_QOS, 
            .pubTopic = DEFAULT_MQTT_PUBLISH_TOPIC, 
            .pubQos = DEFAULT_MQTT_PUBLISH_TOPIC_QOS},
        .ntpConfigData = {
            .ntpEnabled = DEAFULT_NTP_ENABLE,
            .ntpServer1 = DEFAULT_NTP_SERVER_1_URL,
            .ntpServer2 = DEFAULT_NTP_SERVER_2_URL,
            .ntpServer3 = DEFAULT_NTP_SERVER_3_URL,
            .timeZone = DEFAULT_TIME_ZONE,
            .ntpAccepNoExternalTimeService = DEFAULT_ACCEPT_NO_EXTERNAL_TIME_SERVICE,},
        .otaConfigData = {
            .otaEnabled = DEAFULT_OTA_ENABLE,
            .esp32OtaServer1 = DEFAULT_ESP32_OTA_SERVER_1_URL,
            .esp32OtaServer2 = DEFAULT_ESP32_OTA_SERVER_2_URL,
            .esp32OtaServer3 = DEFAULT_ESP32_OTA_SERVER_3_URL,}                
        },
    .DeviceConfigData = {
        .ledstripConfigData = {
            .ledstripEnabled = DEAFULT_LED_STRIP_ENABLE,
            .ledNum = DEAFULT_LED_STRIP_PIXEL_NUM,
            .ledModel = DEAFULT_LED_STRIP_MODLE,
            .btightness = DEAFULT_LED_STRIP_BTIGHTNESS}
        },  
    .projectConfigData = {
        .ledStripIndicationConfigData = {
            .allowOrderOverwriteLocation = DEAFULT_USER_OVERWRITE_LOCATION_SET,
            .indicationModle = DEFAULT_LED_STRIP_INDICATION_MODLE,
            .orderOwnLedMaxNum = DEFAULT_LED_STRIP_INDICATION_OWN_LED_MAXNUM,
            .colorGreen = DEFAULT_LED_STRIP_INDICATION_COLOR_GREEN,
            .colorYellow = DEFAULT_LED_STRIP_INDICATION_COLOR_YELLOW,
            .colorRed = DEFAULT_LED_STRIP_INDICATION_COLOR_RED,
            .colorBlue = DEFAULT_LED_STRIP_INDICATION_COLOR_BLUE,}
        },
};

/**
 * @brief  使用cjsonx_struct2str_preallocated将结构体数据转换为字符串，并打印到串口输出
 * @param  tag
 * @param  nvsData 转换的结构体
 */
void dumpNvsData(const char *tag, NvsData_t nvsData)
{
    char buf[MAX_CONFIG_LEN] = {0};
    int ret = cjsonx_struct2str_preallocated(buf, sizeof(buf), &nvsData, NvsData_reflection);
    if (ret == ERR_CJSONX_NONE)
    {
        ESP_LOGI(tag, "NvsData: %s", buf);
    }
    else
    {
        ESP_LOGE(tag, "Failed to dump NvsData.");
    }
}

void dumpSysStateInfo(const char *tag, SysStateInfo_t sysStateInfo)
{
    char buf[MAX_CONFIG_LEN] = {0};
    int ret = cjsonx_struct2str_preallocated(buf, sizeof(buf), &sysStateInfo, SysStateInfo_reflection);
    if (ret == ERR_CJSONX_NONE)
    {
        ESP_LOGI(tag, "SysStateInfo: %s", buf);
    }
    else
    {
        ESP_LOGE(tag, "Failed to dump SysStateInfo.");
    }
}

/**
 * @brief  把结构体转化为JSON字符串判定是否相等
 * @param  struct1
 * @param  struct2
 * @param  tbl
 * @return true  结构体相等
 * @return false 结构体不相等
 */
bool configStructCmp(void *struct1, void *struct2, const cjsonx_reflect_t *tbl)
{
    char struct1JsonBuf[MAX_CONFIG_LEN / 4] = {0};
    char struct2JsonBuf[MAX_CONFIG_LEN / 4] = {0};
    int ret;
    ret = cjsonx_struct2str_preallocated(struct1JsonBuf, sizeof(struct1JsonBuf), struct1, tbl);
    if (ret != ERR_CJSONX_NONE)
    {
        ESP_LOGE(TAG, "Failed to convert struct1 to JSON.");
    }
    // ESP_LOGI(TAG, "struct1JsonBuf = %s", struct1JsonBuf);
    ret = cjsonx_struct2str_preallocated(struct2JsonBuf, sizeof(struct2JsonBuf), struct2, tbl);
    if (ret != ERR_CJSONX_NONE)
    {
        ESP_LOGE(TAG, "Failed to convert struct2JsonBuf to JSON.");
    }
    // ESP_LOGI(TAG, "struct2JsonBuf = %s", struct2JsonBuf);
    if (strcmp(struct1JsonBuf, struct2JsonBuf) != 0)
    {
        return false;
    }
    return true;
}

/**
 * @brief 获取固件编译日期
 * @return const char*
 */
const char *getCompiledDate()
{
    // __DATE__ result: Apr 23 2020, Apr232020 is 9 chars
    // 2020-04-23\0 is 9 chars
    static char buf[11] = {0};
    static const char *month_buf[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int month = 0;
    int i = 0;

    if (buf[0] == 0)
    {
        const char *cp_date = __DATE__;
        // store year to start of buf
        for (i = 0; i < 4; i++)
        {
            buf[i] = *(cp_date + 7 + i);
        }
        // store date to start of buf, but decade is ' ' when day little than 10
        for (i = 0; i < 12; i++)
        {
            // When _month[i] and cp_date store the same value in memory, month is i+1.
            if ((month_buf[i][0] == (cp_date[0])) &&
                (month_buf[i][1] == (cp_date[1])) &&
                (month_buf[i][2] == (cp_date[2])))
            {
                month = i + 1;
                break;
            }
        }
        buf[4] = '-';
        buf[5] = month / 10 % 10 + '0';
        buf[6] = month % 10 + '0';
        buf[7] = '-';
        // judge day is little than 10
        if (cp_date[4] == ' ')
        {
            buf[8] = '0';
        }
        else
        {
            buf[8] = cp_date[4];
        }
        buf[9] = cp_date[5];
    }
    return buf;
}

int bytes2HexStr(uint8_t *bytes, int len, uint8_t *hexStr)
{
    uint8_t item[2];
    int i, j, dlen;

    dlen = 0;
    for (i = 0; i < len; i++)
    {
        item[0] = (bytes[i] & 0xF0) >> 4;
        item[1] = (bytes[i] & 0x0F);
        for (j = 0; j < 2; j++)
        {
            if (item[j] < 10)
            {
                item[j] += '0';
            }
            else
            {
                item[j] = item[j] - 10 + 'A';
            }
            hexStr[dlen++] = item[j];
        }
    }
    dlen += 1;
    return dlen;
}

int hexStr2Bytes(uint8_t *hexStr, int len, uint8_t *bytes)
{
    int i, j, dlen;
    uint8_t item[2];

    dlen = len / 2;
    if (len % 2)
    {
        return 0;
    }
    for (i = 0; i < dlen; i++)
    {
        item[0] = *hexStr++;
        item[1] = *hexStr++;
        for (j = 0; j < 2; j++)
        {
            if (item[j] <= 'F' && item[j] >= 'A')
            {
                item[j] = item[j] - 'A' + 10;
            }
            else if (item[j] <= 'f' && item[j] >= 'a')
            {
                item[j] = item[j] - 'a' + 10;
            }
            else if (item[j] >= '0' && item[j] <= '9')
            {
                item[j] = item[j] - '0';
            }
            else
            {
                return 0;
            }
        }
        bytes[i] = item[0] << 4;
        bytes[i] |= item[1];
    }
    return dlen;
}

/**
 * @brief  字符串转数字
 * @param  str
 * @return uint32_t
 */
uint32_t str2num(char *str)
{
    return (uint32_t)atoi(str);
}

/**
 * @brief  获取json元素
 * @param  data             
 * @param  string           
 * @return cJSON* 
 */
cJSON *getJSONobj(cJSON *data, const char *const string)
{
    cJSON *jsonObj = NULL;
    jsonObj = cJSON_GetObjectItem(data, string);
    if (jsonObj == NULL)
    {
        ESP_LOGE(TAG, "JSON Parse failed. Unable to obtain [ %s ]", string);
        return NULL;
    }
    else
    {
        return jsonObj;
    }
}
