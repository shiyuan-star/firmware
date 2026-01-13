/**
 * @file main.c
 * @brief 主程序
 * @version 1.0
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "common.h"
// 全局变量
static char *TAG = "MAIN"; // main文件LOG标签

/**
 * @brief 程序入口
 */
void app_main(void)
{
    ESP_LOGI(TAG, "--------------IoTerminal_SSAIS_FIRMWARE [%s] --------------", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Build timestamp: %s %s", getCompiledDate(), __TIME__);
    g_sysStateInfo.bootFreeHeapSize = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    ESP_LOGI(TAG, "system boot completed free heap size: %ld bytes", g_sysStateInfo.bootFreeHeapSize);
    static uint8_t staMac[6] = {0};
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(staMac));
    sprintf(g_sysStateInfo.staMac, "%02x:%02x:%02x:%02x:%02x:%02x", staMac[0], staMac[1], staMac[2], staMac[3], staMac[4], staMac[5]);

    ESP_LOGI(TAG, "---------------------------Init NVS---------------------------");
    strcat(g_defaultNvsData.networkConfigData.mqttConfigData.subTopic, g_sysStateInfo.staMac); // 订阅主题尾部附加设备wifi Ap的mac地址
    strcat(g_defaultNvsData.networkConfigData.mqttConfigData.pubTopic, g_sysStateInfo.staMac);
    ESP_ERROR_CHECK(readNvsDataConfig(&g_nvsData));
    dumpNvsData(TAG, g_nvsData);

    ESP_LOGI(TAG, "------------------Init StateLed | AlarmLed | DOUT-------------------");
    outputGpioInit();
    inputGpioInit();
    ESP_ERROR_CHECK(stateLedIndicatorInit());
    ESP_ERROR_CHECK(alarmLedIndicatorInit());
    ESP_ERROR_CHECK(digitalOutputIndicatorInit());
    alarmLedStateSet(ALARM_STATE_NET_DISCONNECT); // 红灯闪烁,等待网络连接完成
    ESP_LOGI(TAG, "--------------------------Init Screen-------------------------");
    SCREEN_POWER_ON;
    g_screenStateMutex = xSemaphoreCreateMutex(); // 创建屏幕状态互斥信号量
    screenInit(CONFIG_SCREEN_UART_BAUDRATE, CONFIG_SCREEN_UART_TX_PIN, CONFIG_SCREEN_UART_RX_PIN);
    // 创建屏幕命令任务，等待屏幕初始化完成，监听屏幕指令
    xTaskCreate(screenCmdRecvTask, "screenCmdRecvTask", 8192, NULL, SCREEN_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);

    ESP_LOGI(TAG, "------------------Init DIN-------------------");
    initDinButton();

    ESP_LOGI(TAG, "--------------------------Init MQTT---------------------------");
    g_mqttRecvDataQueueHandler = xQueueCreateWithCaps(MQTT_RECEIVE_QUEUE_LEN, sizeof(MqttReceiveData_t), MALLOC_CAP_SPIRAM);
    g_mqttPubDataQueueHandler = xQueueCreateWithCaps(MQTT_PUBISH_QUEUE_LEN, sizeof(MqttPublishData_t), MALLOC_CAP_SPIRAM);
    mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_SYSTEM_REBOOT, NOTIFY_SYSTEM_REBOOT, "version", FIRMWARE_VERSION);
    xTaskCreate(mqttTask, "mqttTask", 16384, NULL, MQTT_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);

    ESP_LOGI(TAG, "--------------------------Init Network------------------------");
    g_sysStateInfoMetex = xSemaphoreCreateMutex(); // 创建系统状态互斥信号量
    NETWORT_STATE_UPDATE(NET_DISCONNECT);
    xTaskCreate(networkTask, "networkTask", 8192, NULL, NETWORK_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);

    // ESP_LOGI(TAG, "--------------------------Init Modbus-------------------------");
    // if (g_nvsData.DeviceConfigData.rs485ConfigData.rs485Enabled)
    // {
    //     xTaskCreate(modbusTask, "modbusTask", 8192, NULL, MODBUS_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);
    // }

    if (g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled)
    {
        ESP_LOGI(TAG, "--------------------------Init Ledstrip-------------------------");
        g_ledstripRmtHandleMetex = xSemaphoreCreateMutex(); // 创建LED灯带句柄互斥信号量
        g_ledstripRmtHandle = LedStripInit(g_nvsData.DeviceConfigData.ledstripConfigData);
        LEDSTRIP_CLEAR; // 带电复位的情况下，清除残留的灯珠
        LEDSTRIP_REFRESH;
    }

    ESP_LOGI(TAG, "--------------------------Init OTA---------------------------");
    g_startOtaTaskSemphHandle = xSemaphoreCreateBinary();
    xSemaphoreTake(g_startOtaTaskSemphHandle, 0);
    xTaskCreate(otaTask, "otaTask", 8192, NULL, OTA_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);

    ESP_LOGI(TAG, "--------------------------Init BUSINESS-------------------------");
    if (g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled)
    {
        g_ledStripBoxDataQueueHandler = xQueueCreateWithCaps(LED_STRIP_INDICATION_MAX_ORDER_BOX_SIZE, sizeof(BoxData_t), MALLOC_CAP_SPIRAM);
        g_ledStripBoxDataSemphHandle = xSemaphoreCreateBinary();
        xSemaphoreTake(g_ledStripBoxDataSemphHandle, 0);
        xTaskCreate(ledStripIndicationTask, "shelveTask_1", 8192, NULL, LEDSTRIP_INDICATION_TASK_PRIVILEGE | portPRIVILEGE_BIT, NULL);
    }
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
