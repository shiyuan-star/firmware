/**
 * @file networkTask.c
 * @brief 网络管理任务
 * @version 1.0
 * @date 2024-01-05
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "user_tasks.h"
#include "network.h"

static char *TAG = "NETWORK";
static EthConfigData_t ethConfigData;
static WifiConfigData_t wifiConfigData;
static NetTaskState_t s_netTaskState = NET_TASK_INIT;
static NetTaskState_t s_lastNetTaskState = NET_TASK_INIT;
esp_mqtt_client_handle_t g_mqttClientHandle; // MQTT句柄

/**
 * @brief  网络连接处理任务
 * @param  pvParameters
 */
void networkTask(void *pvParameters)
{
    ethConfigData.ethernetEnabled = false;
    wifiConfigData.wifiEnabled = false;
    for (;;)
    {
        if (wifiConfigData.wifiEnabled != g_nvsData.networkConfigData.wifiConfigData.wifiEnabled)
        {
            // Wi-Fi ON->OFF
            if (wifiConfigData.wifiEnabled == true && g_nvsData.networkConfigData.wifiConfigData.wifiEnabled == false)
            {
                wifiConfigData.wifiEnabled = g_nvsData.networkConfigData.wifiConfigData.wifiEnabled;
                switchNetTaskState(NET_TASK_QUIT);
                networkTaskLoop();
                switchNetTaskState(NET_TASK_INIT);
                switchWifiConnMgr(WIFI_MGR_QUIT);
                wifiConnManagerLoop();
            }
            // Wi-Fi OFF->ON
            if (wifiConfigData.wifiEnabled == false && g_nvsData.networkConfigData.wifiConfigData.wifiEnabled == true)
            {
                wifiConfigData.wifiEnabled = g_nvsData.networkConfigData.wifiConfigData.wifiEnabled;
                switchWifiConnMgr(WIFI_MGR_INIT);
            }
        }
        if (ethConfigData.ethernetEnabled != g_nvsData.networkConfigData.ethConfigData.ethernetEnabled)
        {
            // Ethernet ON->OFF
            if (ethConfigData.ethernetEnabled == true && g_nvsData.networkConfigData.ethConfigData.ethernetEnabled == false)
            {
                ethConfigData.ethernetEnabled = g_nvsData.networkConfigData.ethConfigData.ethernetEnabled;
                switchNetTaskState(NET_TASK_QUIT);
                networkTaskLoop();
                switchNetTaskState(NET_TASK_INIT);
                switchEthConnMgr(ETH_MGR_QUIT);
                ethConnManagerLoop();
            }
            // Ethernet OFF->ON
            if (ethConfigData.ethernetEnabled == false && g_nvsData.networkConfigData.ethConfigData.ethernetEnabled == true)
            {
                ethConfigData.ethernetEnabled = g_nvsData.networkConfigData.ethConfigData.ethernetEnabled;
                switchEthConnMgr(ETH_MGR_INIT);
            }
        }
        if (wifiConfigData.wifiEnabled == true)
        {
            // 判断WiFi参数是不是有变化
            if (strcmp(wifiConfigData.ssid, g_nvsData.networkConfigData.wifiConfigData.ssid) != 0 ||
                strcmp(wifiConfigData.password, g_nvsData.networkConfigData.wifiConfigData.password) != 0 ||
                wifiConfigData.maximumRetry != g_nvsData.networkConfigData.wifiConfigData.maximumRetry)
            {
                wifiConfigData = g_nvsData.networkConfigData.wifiConfigData;
                ESP_LOGI(TAG, "WiFi config changed [Enabled:%d, AP:%s, PASSWORD:%s, RETRY:%d].",
                         wifiConfigData.wifiEnabled, wifiConfigData.ssid, wifiConfigData.password, wifiConfigData.maximumRetry);
                switchNetTaskState(NET_TASK_QUIT);
                networkTaskLoop();
                switchNetTaskState(NET_TASK_INIT);
                wifiConnMgrInit(&wifiConfigData);
            }
            // WiFi连接调度
            wifiConnManagerLoop();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if (ethConfigData.ethernetEnabled == true)
        {
            // 判断ETH参数是不是有变化
            if (strcmp(ethConfigData.staticIp, g_nvsData.networkConfigData.ethConfigData.staticIp) != 0 ||
                strcmp(ethConfigData.netMask, g_nvsData.networkConfigData.ethConfigData.netMask) != 0 ||
                strcmp(ethConfigData.gateway, g_nvsData.networkConfigData.ethConfigData.gateway) != 0 ||
                ethConfigData.ethDhcpEnabled != g_nvsData.networkConfigData.ethConfigData.ethDhcpEnabled)
            {
                ethConfigData = g_nvsData.networkConfigData.ethConfigData;
                ESP_LOGI(TAG, "ETH config changed [Enabled:%d, DHCP:%d, Static IP:%s, NetMask:%s, Gateway:%s]",
                         ethConfigData.ethernetEnabled, ethConfigData.ethDhcpEnabled, ethConfigData.staticIp, ethConfigData.netMask, ethConfigData.gateway);
                switchNetTaskState(NET_TASK_QUIT);
                networkTaskLoop();
                switchNetTaskState(NET_TASK_INIT);
                ethConnMgrInit(&ethConfigData);
            }
            // Ethernet连接调度
            ethConnManagerLoop();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        if ((g_sysStateInfo.network == WIFI_CONNECT) || (g_sysStateInfo.network == ETH_CONNECT))
        {
            // 处理网络应用
            networkTaskLoop();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}

NetTaskState_t getNetTaskState()
{
    return s_netTaskState;
}

/**
 * @brief 切换网络任务状态
 * @param  netTaskState
 */
void switchNetTaskState(NetTaskState_t netTaskState)
{
    if (netTaskState != s_netTaskState)
    {
        s_lastNetTaskState = s_netTaskState;
        s_netTaskState = netTaskState;
        // ESP_LOGI(TAG, "NetTaskState %d -> %d", s_lastNetTaskState, s_netTaskState);
    }
}

/**
 * @brief  SNTP 初始化
 */
void sntpInit(NtpConfigData_t *ntpconfigData)
{
    ESP_LOGI(TAG, "SNTP Init");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    ESP_LOGI(TAG, "SNTP Server 1: %s", ntpconfigData->ntpServer1);
    ESP_LOGI(TAG, "SNTP Server 2: %s", ntpconfigData->ntpServer2);
    ESP_LOGI(TAG, "SNTP Server 3: %s", ntpconfigData->ntpServer3);
    if (strlen(ntpconfigData->ntpServer1) > 0)
    {
        esp_sntp_setservername(0, ntpconfigData->ntpServer1);
    }
    if (strlen(ntpconfigData->ntpServer2) > 0)
    {
        esp_sntp_setservername(1, ntpconfigData->ntpServer2);
    }
    if (strlen(ntpconfigData->ntpServer3) > 0)
    {
        esp_sntp_setservername(2, ntpconfigData->ntpServer3);
    }
    setenv("TZ", ntpconfigData->timeZone, 1);
    tzset();
    esp_sntp_init();
}

/**
 * @brief 打印系统时间
 */
void showSystemTime()
{
    time_t now = 0;
    struct tm *timeinfo;
    char timeStr[64] = {0};

    time(&now);
    timeinfo = localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
    ESP_LOGI(TAG, "System time: %s", timeStr);
}

/**
 * @brief 检测所给字符串时候是IPV4地址
 * @param  domain
 * @return 是IPV4地址返回 1 否则返回 -1
 */
int checkIsIpv4(char *domain)
{
    struct in_addr s;
    char IPdotdec[20] = {0};

    if ((strlen(domain) == 0) || (strlen(domain) > 128))
    {
        ESP_LOGE(TAG, "Invalid domain length!");
        return -1;
    }
    if (inet_pton(AF_INET, domain, (void *)&s) == 1)
    {
        inet_ntop(AF_INET, (void *)&s, IPdotdec, 16);
        return 1;
    }
    else
    {
        return -1;
    }
}

void networkTaskLoop()
{

    switch (s_netTaskState)
    {
    case NET_TASK_INIT:
    {
        switchNetTaskState(NET_TASK_SNTP_INIT);
        break;
    }

    case NET_TASK_SNTP_INIT:
    {
        if (g_nvsData.networkConfigData.ntpConfigData.ntpEnabled == true)
        {
            ESP_LOGI(TAG, "SNTP started");
            sntpInit(&g_nvsData.networkConfigData.ntpConfigData);
            showSystemTime();
            ESP_LOGI(TAG, "SNTP Waiting for system time to be set ...");
            switchNetTaskState(NET_TASK_SNTP_CHECK);
        }
        else
        {
            switchNetTaskState(NET_TASK_MQTT_INIT);
        }
        break;
    }

    case NET_TASK_SNTP_CHECK:
    {
        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED)
        {
            ESP_LOGI(TAG, "SNTP sync system time completed");
            switchNetTaskState(NET_TASK_SNTP_READY);
        }
        break;
    }

    case NET_TASK_SNTP_READY:
    {
        showSystemTime();
        ESP_LOGW(TAG, "SNTP ready");
        switchNetTaskState(NET_TASK_MQTT_INIT);
        break;
    }

    case NET_TASK_MQTT_INIT:
    {
        if (g_nvsData.networkConfigData.mqttConfigData.mqttEnabled == true)
        {
            ESP_LOGI(TAG, "MQTT started");
            g_mqttClientHandle = mqttInit(&g_nvsData.networkConfigData.mqttConfigData);
            switchNetTaskState(NET_TASK_MQTT_CHECK);
        }
        else
        {
            switchNetTaskState(NET_TASK_READY);
        }
        break;
    }

    case NET_TASK_MQTT_CHECK:
    {
        if (getMqttState() == MQTT_CONNECT)
        {
            esp_mqtt_client_subscribe(g_mqttClientHandle, g_nvsData.networkConfigData.mqttConfigData.subTopic, g_nvsData.networkConfigData.mqttConfigData.subQos);
            switchNetTaskState(NET_TASK_MQTT_READY);
        }
        break;
    }

    case NET_TASK_MQTT_READY:
    {
        if (getMqttState() == MQTT_SUBSCRIBE)
        {
            switchMqttState(MQTT_READY);
            alarmLedStateSet(ALARM_STATE_SYSTEAM_STAND_BY); // 网络连接完成关闭红灯闪烁
            ESP_LOGW(TAG, "MQTT ready");
            switchNetTaskState(NET_TASK_READY);
            mqttDefaultTopicPubNumMsg(MQTT_CONTROL_TYPE_MQTT_STATE, NOTIFY_MQTT_OFFLINE_RECONNECTION_COUNT, "count", g_sysStateInfo.mqttOfflineReconnectCount);
            g_sysStateInfo.mqttOfflineReconnectCount++;
        }
        break;
    }

    case NET_TASK_READY:
    {
        break;
    }

    case NET_TASK_QUIT:
    {
        if (s_lastNetTaskState != s_netTaskState)
        {
            if (g_nvsData.networkConfigData.mqttConfigData.mqttEnabled == true)
            {
                if (s_lastNetTaskState >= NET_TASK_SNTP_CHECK)
                {
                    esp_sntp_stop();
                }
                ESP_LOGW(TAG, "SNTP stopped");
            }
            if (g_nvsData.networkConfigData.mqttConfigData.mqttEnabled == true)
            {
                if (s_lastNetTaskState >= NET_TASK_READY)
                {
                    esp_mqtt_client_unsubscribe(g_mqttClientHandle, g_nvsData.networkConfigData.mqttConfigData.subTopic);
                }
                if (s_lastNetTaskState >= NET_TASK_MQTT_CHECK)
                {
                    ESP_ERROR_CHECK(esp_mqtt_client_unregister_event(g_mqttClientHandle, ESP_EVENT_ANY_ID, mqttEventHandler));
                    ESP_ERROR_CHECK(esp_mqtt_client_stop(g_mqttClientHandle));
                    ESP_ERROR_CHECK(esp_mqtt_client_destroy(g_mqttClientHandle));
                }
                switchMqttState(MQTT_DISCONNECT); 
                ESP_LOGW(TAG, "MQTT stopped");
            }
            s_lastNetTaskState = s_netTaskState;
        }
        break;
    }
    }
}
