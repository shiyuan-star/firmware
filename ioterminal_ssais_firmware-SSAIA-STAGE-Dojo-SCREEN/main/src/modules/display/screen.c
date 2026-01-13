/**
 * @file screen.c
 * @brief 串口屏事件对应
 * @version 1.0
 * @date 2023-12-26
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"
#include "data_type.h"
extern WifiConfigData_t wifiConfigData;
extern EthConfigData_t ethConfigData; // 来自 network_set.c
static uint8_t networkSelect = 0;

static const char *TAG = "SCREEN";
static screen_page_handle_t sysSetPageHandle[SCREEN_SET_PAGE_MAX] = {
    [SCREEN_SYSTEMSET_AND_INFO_PAGE] = {.screenId = SCREEN_SYSTEMSET_AND_INFO_PAGE, .screen_page_handle = systemInfoSetHandle},
    [SCREEN_NETWORK_SET_PAGE] = {.screenId = SCREEN_NETWORK_SET_PAGE, .screen_page_handle = networkSetHandle},
    [SCREEN_PROTOCOL_SET_PAGE] = {.screenId = SCREEN_PROTOCOL_SET_PAGE, .screen_page_handle = protocolSetHandle},
    [SCREEN_MQTT_SET_PAGE] = {.screenId = SCREEN_MQTT_SET_PAGE, .screen_page_handle = mqttSetHandle},
    [SCREEN_NTP_SET_PAGE] = {.screenId = SCREEN_NTP_SET_PAGE, .screen_page_handle = ntpSetHandle},
    [SCREEN_OTA_SET_PAGE] = {.screenId = SCREEN_OTA_SET_PAGE, .screen_page_handle = otaSetHandle},
    [SCREEN_DEVICE_SET_PAGE] = {.screenId = SCREEN_DEVICE_SET_PAGE, .screen_page_handle = deviceSetHandle},
    [SCREEN_LEDSTRIP_SET_PAGE] = {.screenId = SCREEN_LEDSTRIP_SET_PAGE, .screen_page_handle = ledstripSetHandle},
    [SCREEN_LEDSTRIP_DEBUG_PAGE] = {.screenId = SCREEN_LEDSTRIP_DEBUG_PAGE, .screen_page_handle = ledstripDebugHandle},
    [SCREEN_RS485_SET_PAGE] = {.screenId = SCREEN_RS485_SET_PAGE, .screen_page_handle = rs485SetHandle},
    [SCREEN_DIO_SET_PAGE] = {.screenId = SCREEN_DIO_SET_PAGE, .screen_page_handle = dioSetHandle},
    [SCREEN_SCREEN_SET_PAGE] = {.screenId = SCREEN_SCREEN_SET_PAGE, .screen_page_handle = screenSetHandle},
    [SCREEN_PROJECT_SET_PAGE] = {.screenId = SCREEN_PROJECT_SET_PAGE, .screen_page_handle = projectSetHandle},
    [SCREEN_SSAIS_PROJECT_SET_PAGE] = {.screenId = SCREEN_SSAIS_PROJECT_SET_PAGE, .screen_page_handle = ssaisProjectHandle},
    [SCREEN_MESSAGE_DIALOG_PAGE] = {.screenId = SCREEN_MESSAGE_DIALOG_PAGE, .screen_page_handle = messageDialogHandle},
    [SCREEN_CHECK_DIALOG_PAGE] = {.screenId = SCREEN_CHECK_DIALOG_PAGE, .screen_page_handle = checkDialogHandle},
};

/**
 * @brief 屏幕启动更新设置页面信息
 * @param  screenId 屏幕页面
 */
void screenInfoUpdate(uint16_t screenId)
{
    switch (screenId)
    {
    case SCREEN_SYSTEMSET_AND_INFO_PAGE:
        if (xSemaphoreTake(g_sysStateInfoMetex, pdMS_TO_TICKS(1)) == pdTRUE)
        {
            SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_MAC_TEXT, (uint8_t *)g_sysStateInfo.staMac);
            SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_MAC_QRCODE_TEXT, (uint8_t *)g_sysStateInfo.staMac);
            if (g_sysStateInfo.network == NET_DISCONNECT)
            {
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_NETWORK_NAME_TEXT, (uint8_t *)"No Network");
                SetTextClena(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_RSSI_TEXT);
                SetTextClena(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_IP_TEXT);
            }
            else if (g_sysStateInfo.network == ETH_CONNECT)
            {
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_NETWORK_NAME_TEXT, (uint8_t *)"Ethernet");
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_RSSI_TEXT, (uint8_t *)"10/100Mbps");
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_IP_TEXT, (uint8_t *)g_sysStateInfo.ipAddr);
            }
            else if (g_sysStateInfo.network == WIFI_CONNECT)
            {
                char networkNameStr[48] = {0};
                strcat(networkNameStr, "SSID: ");
                strcat(networkNameStr, g_nvsData.networkConfigData.wifiConfigData.ssid);
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_NETWORK_NAME_TEXT, (uint8_t *)networkNameStr);
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_IP_TEXT, (uint8_t *)g_sysStateInfo.ipAddr);
                wifi_ap_record_t wifiApRecord;
                esp_wifi_sta_get_ap_info(&wifiApRecord); // SSID信息获取
                char rssiStr[9];
                sprintf(rssiStr, "%d dBm", wifiApRecord.rssi);
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_RSSI_TEXT, (uint8_t *)rssiStr);
            }

            if (getMqttState() == MQTT_DISCONNECT || getMqttState() == MQTT_QUIT || !g_nvsData.networkConfigData.mqttConfigData.mqttEnabled || getNetTaskState() < NET_TASK_MQTT_READY || getNetTaskState() == NET_TASK_QUIT)
            {
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_MQTT_TEXT, (uint8_t *)"Disconnect");
            }
            else
            {
                SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_MQTT_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.url);
            }

            char runTimeStr[20];
            sprintf(runTimeStr, "%ld min", esp_log_timestamp() / 60000);
            SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_RUNTIME_TEXT, (uint8_t *)runTimeStr);

            double internalRamUse = 1 - ((double)heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / (double)g_sysStateInfo.bootFreeHeapSizeInternal);
            double psramRamUse = 1 - ((double)heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (double)g_sysStateInfo.bootFreeHeapSizePsram);
            char ramUseStr[32] = {0};
            sprintf(ramUseStr, "SRAM: %.2f %% PSRAM: %.2f %%", internalRamUse * 100, psramRamUse * 100);
            SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_SYSTEM_RAM_TEXT, (uint8_t *)ramUseStr);
            xSemaphoreGive(g_sysStateInfoMetex);
        }
        time_t now = 0;
        struct tm *timeinfo;
        char timeStr[64] = {0};
        time(&now);
        timeinfo = localtime(&now);
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
        SetTextValue(SCREEN_SYSTEMSET_AND_INFO_PAGE, SCREEN_CLOCK_TIME_TEXT, (uint8_t *)timeStr);

        break;
    case SCREEN_NETWORK_SET_PAGE:
        SetButtonValue(SCREEN_NETWORK_SET_PAGE, SCREEN_ETH_NETWORK_ENABLE_BUTTON, g_nvsData.networkConfigData.ethConfigData.ethernetEnabled); // 显示以太网使能状态
        SetTextValue(SCREEN_NETWORK_SET_PAGE, SCREEN_ETH_STATIC_IP_TEXT, (uint8_t *)g_nvsData.networkConfigData.ethConfigData.staticIp);
        SetTextValue(SCREEN_NETWORK_SET_PAGE, SCREEN_ETH_NET_MASK_TEXT, (uint8_t *)g_nvsData.networkConfigData.ethConfigData.netMask);
        SetTextValue(SCREEN_NETWORK_SET_PAGE, SCREEN_ETH_GATEWAY_TEXT, (uint8_t *)g_nvsData.networkConfigData.ethConfigData.gateway);
        SetButtonValue(SCREEN_NETWORK_SET_PAGE, SCREEN_ETH_DHCP_ENABLE_BUTTON, g_nvsData.networkConfigData.ethConfigData.ethDhcpEnabled);

        SetButtonValue(SCREEN_NETWORK_SET_PAGE, SCREEN_WIFI_NETWORK_ENABLE_BUTTON, g_nvsData.networkConfigData.wifiConfigData.wifiEnabled); // 显示WiFi使能状态
        SetTextValue(SCREEN_NETWORK_SET_PAGE, SCREEN_WIFI_SSID_TEXT, (uint8_t *)g_nvsData.networkConfigData.wifiConfigData.ssid);           // 显示WiFi
        SetTextValue(SCREEN_NETWORK_SET_PAGE, SCREEN_WIFI_PASS_TEXT, (uint8_t *)g_nvsData.networkConfigData.wifiConfigData.password);       // 显示WiFi密码
        break;
    case SCREEN_MQTT_SET_PAGE:
        SetButtonValue(SCREEN_MQTT_SET_PAGE, SCREEN_MQTT_ENABLE_BUTTON, g_nvsData.networkConfigData.mqttConfigData.mqttEnabled); // 显示MQTT使能状态
        SetTextValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_SERVER_URL_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.url);
        SetTextValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_USERNAME_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.userName);
        SetTextValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_PASS_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.userPassword);
        SetTextValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_SUBTOPIC_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.subTopic);
        SetTextNumValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_SUBTOPIC_QOS_TEXT, g_nvsData.networkConfigData.mqttConfigData.subQos);
        SetTextValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_PUBTOPIC_TEXT, (uint8_t *)g_nvsData.networkConfigData.mqttConfigData.pubTopic);
        SetTextNumValue(SCREEN_MQTT_SET_PAGE, SCRENN_MQTT_PUBTOPIC_QOS_TEXT, g_nvsData.networkConfigData.mqttConfigData.pubQos);
        break;
    case SCREEN_NTP_SET_PAGE:
        SetButtonValue(SCREEN_NTP_SET_PAGE, SCREEN_NTP_ENABLE_BUTTON, g_nvsData.networkConfigData.ntpConfigData.ntpEnabled); // 显示NTP使能状态
        SetTextValue(SCREEN_NTP_SET_PAGE, SCREEN_PROJECT_NTP_SERVER_1_URL_TEXT, (uint8_t *)g_nvsData.networkConfigData.ntpConfigData.ntpServer1);
        SetTextValue(SCREEN_NTP_SET_PAGE, SCREEN_PROJECT_NTP_SERVER_2_URL_TEXT, (uint8_t *)g_nvsData.networkConfigData.ntpConfigData.ntpServer2);
        SetTextValue(SCREEN_NTP_SET_PAGE, SCREEN_PROJECT_NTP_SERVER_3_URL_TEXT, (uint8_t *)g_nvsData.networkConfigData.ntpConfigData.ntpServer3);
        SetTextValue(SCREEN_NTP_SET_PAGE, SCREEN_PROJECT_TIME_ZONE_TEXT, (uint8_t *)g_nvsData.networkConfigData.ntpConfigData.timeZone);
        SetButtonValue(SCREEN_NTP_SET_PAGE, SCREEN_NTP_ACCEPT_NO_EXTERNAL_TIME_SERVICE_BUTTON, g_nvsData.networkConfigData.ntpConfigData.ntpAccepNoExternalTimeService);
        break;
    case SCREEN_OTA_SET_PAGE:
        SetTextValue(SCREEN_OTA_SET_PAGE, SCREEN_OTA_NOW_FIRMWARE_VERSION_TEXT, (uint8_t *)FIRMWARE_VERSION);
        SetTextValue(SCREEN_OTA_SET_PAGE, SCREEN_OTA_SERVER_URL_TEXT, (uint8_t *)g_nvsData.networkConfigData.otaConfigData.esp32OtaServer);
        SetTextValue(SCREEN_OTA_SET_PAGE, SCREEN_OTA_FIRMWARE_FILE_NAME_TEXT, (uint8_t *)g_nvsData.networkConfigData.otaConfigData.firmwareFileName);
        break;
    case SCREEN_LEDSTRIP_SET_PAGE:
        SetButtonValue(SCREEN_LEDSTRIP_SET_PAGE, SCREEN_LEDSTRIP_ENABLE_BUTTON, g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled);
        SetTextNumValue(SCREEN_LEDSTRIP_SET_PAGE, SCREEN_LEDSTRIP_LED_NUM_TEXT, g_nvsData.DeviceConfigData.ledstripConfigData.ledNum);
        SetTextNumValue(SCREEN_LEDSTRIP_SET_PAGE, SCREEN_LEDSTRIP_LED_MODLE_TEXT, g_nvsData.DeviceConfigData.ledstripConfigData.ledModel);
        SetTextNumValue(SCREEN_LEDSTRIP_SET_PAGE, SCREEN_LEDSTRIP_PIXEL_FORMAT_TEXT, g_nvsData.DeviceConfigData.ledstripConfigData.pixelFormat);
        SetTextNumValue(SCREEN_LEDSTRIP_SET_PAGE, SCREEN_LEDSTRIP_BRIGHTNESS_TEXT, g_nvsData.DeviceConfigData.ledstripConfigData.btightness);
        break;
    case SCREEN_RS485_SET_PAGE:
        SetButtonValue(SCREEN_RS485_SET_PAGE, SCREEN_RS485_SET_ENABLE_BUTTON, g_nvsData.DeviceConfigData.rs485ConfigData.rs485Enabled);
        SetTextNumValue(SCREEN_RS485_SET_PAGE, SCREEN_RS485_SET_BAUD_TEXT, g_nvsData.DeviceConfigData.rs485ConfigData.baudrate);
        SetTextNumValue(SCREEN_RS485_SET_PAGE, SCREEN_RS485_SET_DEVICES_NUM_TEXT, g_nvsData.DeviceConfigData.rs485ConfigData.slaveNum);
        SetTextNumValue(SCREEN_RS485_SET_PAGE, SCREEN_RS485_SET_MODLE_TEXT, g_nvsData.DeviceConfigData.rs485ConfigData.slaveModel);
        break;
    case SCREEN_DIO_SET_PAGE:
        SetButtonValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_DIO1_ON_OFF_BUTTON, false);
        SetButtonValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_DIO2_ON_OFF_BUTTON, false);
        SetButtonValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_DIO3_ON_OFF_BUTTON, false);
        SetTextClena(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_1_TEXT);
        SetTextClena(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_2_TEXT);
        SetTextClena(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_3_TEXT);
        SetTextClena(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_SELECT_TEXT);
        SetTextClena(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_DEVICES_SELECT_TEXT);
        break;
    case SCREEN_SSAIS_PROJECT_SET_PAGE:
        break;
    default:
        break;
    }
}

/**
 * @brief 更新屏幕显示页面
 * @param  screen_id
 */
void setScreenPage(uint16_t screen_id)
{
    SetScreen(screen_id);
    SCREEN_ID_UPDATE(screen_id);
}

/**
 * @brief  根据屏幕目前语言设置文本
 * @param  screen_id    画面ID
 * @param  control_id   控件ID
 * @param  chineseStr   中文
 * @param  englishStr   英文
 * @param  japaneseStr   日文
 */
void setTextValueMultilingual(uint16_t screen_id, uint16_t control_id, char *chineseStr, char *englishStr, char *japaneseStr)
{

    switch (g_screenState.language)
    {
    case SCREEN_CHINESE:
        SetTextValue(screen_id, control_id, (uint8_t *)chineseStr);
        break;
    case SCREEN_ENGLISH:
        SetTextValue(screen_id, control_id, (uint8_t *)englishStr);
        break;
    case SCREEN_JAPANESE:
        SetTextValue(screen_id, control_id, (uint8_t *)japaneseStr);
        break;
    default:
        break;
    }
}

/**
 * @brief  处理屏幕发出的串口指令
 * @param  msg     由大彩定义的PCTRL_MSG类型
 * @param  size    命令大小
 * @return esp_err_t
 */
esp_err_t screenCmdRecvHandle(PCTRL_MSG msg, uint16_t size)
{
    uint8_t cmdType = msg->cmd_type;                // 指令类型
    uint8_t ctrlMsg = msg->ctrl_msg;                // 消息的类型
    uint8_t controlType = msg->control_type;        // 控件类型
    uint16_t screenId = PTR2U16(&msg->screen_id);   // 画面ID
    uint16_t controlId = PTR2U16(&msg->control_id); // 控件ID
    uint8_t paramLen = (size - 8 - 4);              // msg->param长度
    static char startled[32] = {0};
    static char endled[32] = {0};
    static int curStart = 0;
    static int curEnd = 0;
    (void)controlType;
    if (cmdType == NOTIFY_CUSTOM_INSTRUCTIONS) // 自定义指令
    {
        switch (ctrlMsg)
        {
        case NOTIFY_LANGUAGE:
            xSemaphoreTake(g_screenStateMutex, portMAX_DELAY);
            g_screenState.language = ((uint8_t *)msg)[3]; // 请检查！ 务必和屏幕LUA脚本存放数据位置一致
            ESP_LOGI(TAG, "NOTIFY_LANGUAGE = %d", g_screenState.language);
            xSemaphoreGive(g_screenStateMutex);
            break;
        default:
            ESP_LOGE(TAG, "Screen customization command not supported  cmdType = %d ctrlMsg = %d", cmdType, ctrlMsg);
            return ESP_ERR_NOT_SUPPORTED;
            break;
        }
        return ESP_OK;
    }
    if (ctrlMsg == MSG_GET_CURRENT_SCREEN) // 画面ID变化通知
    {
        SCREEN_ID_UPDATE(screenId);
        ESP_LOGI(TAG, "NOTIFY_SCREEN = %d", screenId);
        return ESP_OK;
    }
    if (screenId <= SCREEN_SET_PAGE_MAX) // 屏幕端系统设置
    {
        for (size_t i = 0; i <= SCREEN_SET_PAGE_MAX; i++)
        {
            if (screenId == sysSetPageHandle[i].screenId)
            {
                return sysSetPageHandle[i].screen_page_handle(controlId, msg->param, paramLen);
            }
        }
        ESP_LOGE(TAG, "Screen page not found");
        return ESP_ERR_NOT_FOUND; // 未找到匹配的页面处理函数
    }

    switch (cmdType)
    {
    case NOTIFY_CONTROL: // 控件通知处理
        ESP_LOGI(TAG, "screenId = %d", screenId);
        ESP_LOGI(TAG, "controlId = %d", controlId);

        if (screenId == 19)
        {
            WifiManager_t st = getWifiManager();
            ESP_LOGI("WIFI_DBG", "current state = %d", st);
            if (g_sysStateInfo.network != WIFI_CONNECT)
            {
                SetScreen(22);
            }
            mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
        }
        else if (screenId == 20)
        {
            static bool ha = false;
            if (!ha)
            {

                static bool has_run = false;
                static bool has = false;
                if (!has_run)
                {

                    GetControlValue(20, 30);

                    char start[sizeof(startled)] = {1};
                    snprintf(start, sizeof(start), "%.*s", paramLen, (char *)msg->param);
                    curStart = atoi(start);
                    has_run = true;
                }

                else if (!has)
                {

                    GetControlValue(20, 31);

                    char end[sizeof(endled)];
                    int curEnd;

                    snprintf(end, sizeof(end), "400");

                    if (paramLen > 0)
                    {
                        snprintf(end, sizeof(end), "%.*s", paramLen, (char *)msg->param);
                    }

                    curEnd = atoi(end);
                    has = true;
                }

                ha = true;
            }

            ESP_LOGI(TAG, "curStart=%d, curEnd=%d", curStart, curEnd);

            switch (controlId)
            {
            case 30:
            {

                char tmpStart[sizeof(startled)] = {0};
                snprintf(tmpStart, sizeof(tmpStart), "%.*s", paramLen, (char *)msg->param);
                int newStart = atoi(tmpStart);

                ESP_LOGI(TAG, "newStart=%d, curEnd=%d", newStart, curEnd);

                if (endled[0] != '\0' && newStart > curEnd)
                {
                    SetScreen(21);
                    SetTextNumValue(20, 30, curStart);
                    return ESP_OK;
                }

                curStart = newStart;
                strncpy(startled, tmpStart, sizeof(startled) - 1);
                mqttPubScreenTextMsg(17, 1, screenId, controlId,
                                     startled, strlen(startled));
            }
            break;

            case 31:
            {

                char tmpEnd[sizeof(endled)] = {0};
                snprintf(tmpEnd, sizeof(tmpEnd), "%.*s", paramLen, (char *)msg->param);
                int newEnd = atoi(tmpEnd);

                ESP_LOGI(TAG, "curStart=%d, newEnd=%d", curStart, newEnd);

                if (startled[0] != '\0' && curStart > newEnd)
                {
                    SetScreen(21);
                    SetTextNumValue(20, 31, curEnd);
                    return ESP_OK;
                }

                curEnd = newEnd;
                strncpy(endled, tmpEnd, sizeof(endled) - 1);
                mqttPubScreenTextMsg(17, 1, screenId, controlId,
                                     endled, strlen(endled));
            }
            break;

            case 8:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            case 11:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            case 12:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            case 13:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            case 14:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            case 15:
                mqttPubScreenCtrlMsg(16, 1, screenId, controlId, msg->param[1]);
                break;
            }

            return ESP_OK;
        }
       
    case NOTIFY_SCREEN_BOOT_OVER:
        xSemaphoreTake(g_screenStateMutex, portMAX_DELAY);
        g_screenState.connectState = true;
        xSemaphoreGive(g_screenStateMutex);
        return ESP_OK;
    default:
        return ESP_ERR_INVALID_ARG;
        break;
    }
    return ESP_OK;
}
