/**
 * @file wireless.c
 * @brief WiFi 驱动与状态机
 * @author ChenJinBo (VP01130@globalymc.com)
 * @version 1.0
 * @date 2023-12-22
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "wireless.h"

static const char *TAG = "WIRELESS";

/* FreeRTOS 的WiFi事件组*/
static EventGroupHandle_t s_wifi_event_group;
static WifiManager_t s_wifiManager = WIFI_MGR_INIT;
static WifiManager_t s_lastWifiManager = WIFI_MGR_INIT;
static esp_event_handler_instance_t s_instance_any_id;
static esp_event_handler_instance_t s_instance_got_ip;
static esp_netif_t *s_esp_netif;
static wifi_config_t s_wifi_config = {
    .sta = {
        .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
    },
};
static uint16_t wifiConnectRetry = 0;
static uint16_t wifiMaximumRetry = 0;

/**
 * @brief WIFI事件回调函数
 * @param  arg
 * @param  event_base
 * @param  event_id
 * @param  event_data
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        NETWORT_STATE_UPDATE(NET_DISCONNECT);
        if (wifiConnectRetry < wifiMaximumRetry)
        {
            esp_wifi_connect();
            wifiConnectRetry++;
            ESP_LOGI(TAG, "Retry to connect AP (%d / %d).", wifiConnectRetry, wifiMaximumRetry);
        }
        else // 超过WiFi重试次数
        {
            ESP_LOGE(TAG, "Connect to the AP failed.");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        wifiConnectRetry = 0;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: %d.%d.%d.%d, MASK: %d.%d.%d.%d, GW: %d.%d.%d.%d", IP2STR(&event->ip_info.ip), IP2STR(&event->ip_info.netmask), IP2STR(&event->ip_info.gw));
        if (xSemaphoreTake(g_sysStateInfoMetex, portMAX_DELAY) == pdTRUE)
        {
            sprintf(g_sysStateInfo.ipAddr, IPSTR, IP2STR(&event->ip_info.ip));
            g_sysStateInfo.network = WIFI_CONNECT;
            xSemaphoreGive(g_sysStateInfoMetex);
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief 获取WiFi连接状态机状态
 * @return WifiManager_t
 */
WifiManager_t getWifiManager()
{
    return s_wifiManager;
}

/**
 * @brief  WiFi连接状态机初始化
 * @param  wifiConfigData
 */
void wifiConnMgrInit(WifiConfigData_t *wifiConfigData)
{
    wifiConnectRetry = 0;
    wifiMaximumRetry = wifiConfigData->maximumRetry;
    strcpy((char *)&s_wifi_config.sta.ssid, wifiConfigData->ssid);
    strcpy((char *)&s_wifi_config.sta.password, wifiConfigData->password);

    if (getWifiManager() != WIFI_MGR_INIT)
    {
        switchWifiConnMgr(WIFI_MGR_QUIT);
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
        while (s_lastWifiManager != s_wifiManager)
        {
            wifiConnManagerLoop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    switchWifiConnMgr(WIFI_MGR_INIT);
}

/**
 * @brief  切换WiFi状态机事件组
 * @param  wifiMgr
 */
void switchWifiConnMgr(WifiManager_t wifiMgr)
{
    if (wifiMgr != s_wifiManager)
    {
        s_lastWifiManager = s_wifiManager;
        s_wifiManager = wifiMgr;
        // ESP_LOGI(TAG, "WifiManager %d -> %d", s_lastWifiManager, s_wifiManager);
    }
}

/**
 * @brief WiFi连接状态机循环
 */
void wifiConnManagerLoop()
{
    switch (s_wifiManager)
    {
    case WIFI_MGR_INIT:
    {
        ESP_LOGW(TAG, "Wireless started");
        g_sysStateInfo.network = NET_DISCONNECT;
        s_wifi_event_group = xEventGroupCreate();
        switchWifiConnMgr(WIFI_MGR_NETIF);
        break;
    }

    case WIFI_MGR_NETIF:
    {
        ESP_ERROR_CHECK(esp_netif_init());
        switchWifiConnMgr(WIFI_MGR_EVENT_LOOP);
        break;
    }

    case WIFI_MGR_EVENT_LOOP:
    {
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        switchWifiConnMgr(WIFI_MGR_WIFI_STA);
        break;
    }

    case WIFI_MGR_WIFI_STA:
    {
        s_esp_netif = esp_netif_create_default_wifi_sta();
        switchWifiConnMgr(WIFI_MGR_LOW_INIT);
        break;
    }

    case WIFI_MGR_LOW_INIT:
    {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));
        switchWifiConnMgr(WIFI_MGR_EVENT_HANDLER);
        break;
    }

    case WIFI_MGR_EVENT_HANDLER:
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &s_instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &s_instance_got_ip));
        switchWifiConnMgr(WIFI_MGR_MODE);
        break;
    }

    case WIFI_MGR_MODE:
    {
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        switchWifiConnMgr(WIFI_MGR_CONFIG);
        break;
    }

    case WIFI_MGR_CONFIG:
    {
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &s_wifi_config));
        switchWifiConnMgr(WIFI_MGR_START);
        break;
    }

    case WIFI_MGR_START:
    {
        ESP_ERROR_CHECK(esp_wifi_start());
        ESP_LOGI(TAG, "Start to connect ssid: %s password: %s", s_wifi_config.sta.ssid, s_wifi_config.sta.password);
        switchWifiConnMgr(WIFI_MGR_CHECK);
        break;
    }

    case WIFI_MGR_CHECK:
    {
        /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
         * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(1));
        if (bits != 0)
        {
            if (bits & WIFI_CONNECTED_BIT)
            {
                ESP_LOGI(TAG, "Success to connect to AP: %s PASSWORD: %s", s_wifi_config.sta.ssid, s_wifi_config.sta.password);
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_NETWORK_STATE, NOTIFY_NETWORK_IP_ADDR, "ip", g_sysStateInfo.ipAddr);
                ESP_LOGW(TAG, "Wireless ready.");
                switchWifiConnMgr(WIFI_MGR_READY);
            }
            else if (bits & WIFI_FAIL_BIT)
            {
                ESP_LOGE(TAG, "Failed to connect to AP: %s, PASSWORD: %s, try again.", s_wifi_config.sta.ssid, s_wifi_config.sta.password);
                xEventGroupClearBits(s_wifi_event_group, WIFI_FAIL_BIT);
                wifiConnectRetry = 0;
                esp_wifi_connect();
            }
            else
            {
                ESP_LOGE(TAG, "Unexpected WiFi event=%ld.", bits);
            }
        }
        break;
    }

    case WIFI_MGR_READY:
    {
        switchWifiConnMgr(WIFI_MGR_CHECK);
        break;
    }

    case WIFI_MGR_QUIT:
    {
        if (s_lastWifiManager != s_wifiManager)
        {
            wifiConnectRetry = 0;
            g_sysStateInfo.network = NET_DISCONNECT;
            if (s_lastWifiManager >= WIFI_MGR_START)
            {
                ESP_ERROR_CHECK(esp_wifi_stop());
            }
            if (s_lastWifiManager >= WIFI_MGR_EVENT_HANDLER)
            {
                ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, s_instance_any_id));
                ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, s_instance_got_ip));
            }
            if (s_lastWifiManager >= WIFI_MGR_LOW_INIT)
            {
                ESP_ERROR_CHECK(esp_wifi_deinit());
            }
            if (s_lastWifiManager >= WIFI_MGR_WIFI_STA)
            {
                esp_netif_destroy_default_wifi(s_esp_netif);
                s_esp_netif = NULL;
            }
            if (s_lastWifiManager >= WIFI_MGR_EVENT_LOOP)
            {
                esp_event_loop_delete_default();
            }
            if (s_lastWifiManager >= WIFI_MGR_NETIF)
            {
                esp_netif_deinit();
            }
            if (s_lastWifiManager >= WIFI_MGR_INIT)
            {
                vEventGroupDelete(s_wifi_event_group);
            }
            s_lastWifiManager = s_wifiManager;
            ESP_LOGW(TAG, "Wireless stopped");
        }
        break;
    }
    }
}
