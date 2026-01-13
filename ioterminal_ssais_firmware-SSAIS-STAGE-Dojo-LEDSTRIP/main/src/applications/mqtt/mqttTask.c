/**
 * @file mqttTask.c
 * @brief mqtt驱动与状态机
 * @version 1.0
 * @date 2024-01-09
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "user_tasks.h"
#include "mqtt.h"
#include "esp_crt_bundle.h"

static char *TAG = "MQTT";
static uint16_t s_mqttConnectRetry = 0, s_mqttMaximumRetry = 0;
static MqttState_t s_mqttState = MQTT_DISCONNECT;
static MqttState_t s_lastMqttState = MQTT_DISCONNECT;
QueueHandle_t g_mqttRecvDataQueueHandler; // MQTT 数据接收队列
QueueHandle_t g_mqttPubDataQueueHandler;  // MQTT 数据发送队列
static mqtt_cmd_handle_t s_sysSetPageHandle[MQTT_CONTROL_TYPE_CLASSIFY_MAX] = {
    [MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_CONTROL] = {.mqttControlTypeClassify = MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_CONTROL,
                                                   .minClassifyNum = MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SCREEN_CONTROL,
                                                   .maxClassifyNum = MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SCREEN_CONTROL,
                                                   .mqtt_cmd_handle = mqttSetScreenControlHandle},
    [MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_STATE] = {.mqttControlTypeClassify = MQTT_CONTROL_TYPE_CLASSIFY_SCREEN_STATE,
                                                 .minClassifyNum = MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SCREEN_STATE,
                                                 .maxClassifyNum = MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SCREEN_STATE,
                                                 .mqtt_cmd_handle = mqttSetScreenStateHandle},
    [MQTT_CONTROL_TYPE_CLASSIFY_DEVICE] = {.mqttControlTypeClassify = MQTT_CONTROL_TYPE_CLASSIFY_DEVICE,
                                           .minClassifyNum = MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_DEVICES,
                                           .maxClassifyNum = MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_DEVICES,
                                           .mqtt_cmd_handle = mqttSetDeviceHandle},
    [MQTT_CONTROL_TYPE_CLASSIFY_SYSTEM] = {.mqttControlTypeClassify = MQTT_CONTROL_TYPE_CLASSIFY_SYSTEM,
                                           .minClassifyNum = MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_SYSTEAM,
                                           .maxClassifyNum = MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_SYSTEAM,
                                           .mqtt_cmd_handle = mqttSetSystemHandle},
    [MQTT_CONTROL_TYPE_CLASSIFY_BUSINESS] = {.mqttControlTypeClassify = MQTT_CONTROL_TYPE_CLASSIFY_BUSINESS,
                                             .minClassifyNum = MQTT_CONTROL_TYPE_MINNUM_CLASSIFY_BUSINESS,
                                             .maxClassifyNum = MQTT_CONTROL_TYPE_MAXNUM_CLASSIFY_BUSINESS,
                                             .mqtt_cmd_handle = mqttSetBusinessHandle},
};
/**
 * @brief 获取MQTT状态
 * @return MqttState_t
 */
MqttState_t getMqttState()
{
    return s_mqttState;
}

/**
 * @brief  切换MQTT状态
 * @param  mqttState
 */
void switchMqttState(MqttState_t mqttState)
{
    if (mqttState != s_mqttState)
    {
        s_lastMqttState = s_mqttState;
        s_mqttState = mqttState;
        // ESP_LOGI(TAG, "MqttState %d -> %d", s_lastMqttState, s_mqttState);
    }
}

/**
 * @brief MQTT初始化
 * @param  mqttConfigData    mqtt配置
 * @return esp_mqtt_client_handle_t
 */
esp_mqtt_client_handle_t mqttInit(MqttConfigData_t *mqttConfigData)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .session.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
        .network.disable_auto_reconnect = false,
        .broker.address.uri = mqttConfigData->url,
        .credentials.username = mqttConfigData->userName,
        .credentials.authentication.password = mqttConfigData->userPassword,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .session.last_will.qos = mqttConfigData->pubQos,
        .session.last_will.topic = mqttConfigData->pubTopic,
        .session.last_will.msg = DEFAULT_MQTT_LAST_WILL_MSG,
        .session.last_will.msg_len = strlen(DEFAULT_MQTT_LAST_WILL_MSG),
        .session.last_will.retain = 0,
    };
    s_mqttConnectRetry = 0;
    s_mqttMaximumRetry = mqttConfigData->maximumRetry;
    switchMqttState(MQTT_DISCONNECT);
    ESP_LOGI(TAG, "MQTT URL: %s", mqttConfigData->url);
    ESP_LOGI(TAG, "MQTT Username: %s Password: %s", mqtt_cfg.credentials.username, mqtt_cfg.credentials.authentication.password);
    ESP_LOGI(TAG, "MQTT Publish Topic = %s  Qos = %d", mqttConfigData->pubTopic, mqttConfigData->pubQos);
    ESP_LOGI(TAG, "MQTT Subscribe Topic = %s Qos = %d", mqttConfigData->subTopic, mqttConfigData->subQos);
    esp_mqtt_client_handle_t clientHandle = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(clientHandle, ESP_EVENT_ANY_ID, mqttEventHandler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(clientHandle));
    return clientHandle;
}

/**
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
void mqttEventHandler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    static MqttReceiveData_t _mqttRecvData;
    static bool disConnectAlarmLedFlag = false; // MQTT断开连接后，是否已经操作三色灯标志
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        disConnectAlarmLedFlag = false;
        switchMqttState(MQTT_CONNECT);
        s_mqttConnectRetry = 0;
        break;
    }

    case MQTT_EVENT_DISCONNECTED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        if (!disConnectAlarmLedFlag) // 断线后还未操作三色灯
        {
            alarmLedStateSet(ALARM_STATE_NET_DISCONNECT);
            disConnectAlarmLedFlag = true;
        }
        switchMqttState(MQTT_DISCONNECT);
        switchNetTaskState(NET_TASK_MQTT_CHECK);
        break;
    }

    case MQTT_EVENT_SUBSCRIBED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        switchMqttState(MQTT_SUBSCRIBE);
        break;
    }

    case MQTT_EVENT_UNSUBSCRIBED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    }

    case MQTT_EVENT_PUBLISHED:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    }

    case MQTT_EVENT_DATA:
    {
        // ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        if (event->topic_len >= MQTT_TOPIC_MAX_LEN)
        {
            ESP_LOGE(TAG, "MQTT topic length exceeds the limit  len = %d", event->topic_len);
            break;
        }
        if (event->total_data_len >= MQTT_RECEIVE_DATA_MAX_LEN)
        {
            ESP_LOGE(TAG, "MQTT data length exceeds the limit  len = %d", event->total_data_len);
            break;
        }
        // ESP_LOGI(TAG, "event->total_data_len:%d    event->data_len:%d ", event->total_data_len, event->data_len);

        if (event->current_data_offset == 0) // 仅第一次MQTT数据事件包含主题
        {
            _mqttRecvData.dataLen = event->total_data_len; // 复制数据长度
            _mqttRecvData.topicLen = event->topic_len;
            memcpy(_mqttRecvData.topic, event->topic, event->topic_len); // 复制接收到的主题
        }
        memcpy(_mqttRecvData.data + event->current_data_offset, event->data, event->data_len); // 复制接收到的数据
        if ((event->current_data_offset + event->data_len) == event->total_data_len)           // 最后一个事件处理完成
        {
            _mqttRecvData.data[event->total_data_len] = '\0';
            xQueueSend(g_mqttRecvDataQueueHandler, &_mqttRecvData, pdMS_TO_TICKS(100));
            memset(&_mqttRecvData, 0, sizeof(_mqttRecvData));
            break;
        }
        else
        {
            break;
        }
    }

    case MQTT_EVENT_BEFORE_CONNECT:
    {
        ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT, msg_id=%d", event->msg_id);
        break;
    }

    case MQTT_EVENT_ERROR:
    {
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        s_mqttConnectRetry++;
        ESP_LOGI(TAG, "Retry to connect MQTT (%d / %d)", s_mqttConnectRetry, s_mqttMaximumRetry);
        if (s_mqttConnectRetry > s_mqttMaximumRetry)
        {
            ESP_LOGE(TAG, "Connect to the MQTT failed");
        }
        break;
    }

    default:
    {
        ESP_LOGI(TAG, "MQTT Other event id:%d", event->event_id);
        break;
    }
    }
}

/**
 * @brief  处理接收的MQTT命令
 * @param  mqttRecvData
 * @return esp_err_t
 */
esp_err_t mqttCmdRecvHandle(MqttReceiveData_t *mqttRecvData)
{
    cJSON *jsonData = NULL;
    cJSON *controlTypeJson = NULL;
    cJSON *cmdTypeJson = NULL;
    cJSON *dataPayloadJson = NULL;
    uint16_t _mqttContorType;
    uint16_t _mqttCmdType;
    esp_err_t err;
    jsonData = cJSON_Parse(mqttRecvData->data); // 解析JSON失败
    if (jsonData == NULL)
    {
        ESP_LOGE(TAG, "JSON parse failed. MQTT data revice is wrong JSON format");
        ESP_LOGE(TAG, "\n%.*s", mqttRecvData->dataLen, mqttRecvData->data);
        cJSON_Delete(jsonData);
        return ESP_FAIL;
    }
    controlTypeJson = cJSON_GetObjectItem(jsonData, "control_type"); // JSON字段没有包含 control_type
    if (controlTypeJson == NULL)
    {
        ESP_LOGE(TAG, "JSON Parse failed. Unable to obtain [ control_type ]");
        ESP_LOGE(TAG, "\n%.*s", mqttRecvData->dataLen, mqttRecvData->data);
        cJSON_Delete(jsonData);
        return ESP_FAIL;
    }
    _mqttContorType = cJSON_GetNumberValue(controlTypeJson);

    cmdTypeJson = cJSON_GetObjectItem(jsonData, "cmd_type"); // JSON字段没有包含 cmd_type
    if (cmdTypeJson == NULL)
    {
        ESP_LOGE(TAG, "JSON Parse failed. Unable to obtain [ cmd_type ]");
        ESP_LOGE(TAG, "\n%.*s", mqttRecvData->dataLen, mqttRecvData->data);
        cJSON_Delete(jsonData);
        return ESP_FAIL;
    }
    _mqttCmdType = cJSON_GetNumberValue(cmdTypeJson);

    dataPayloadJson = cJSON_GetObjectItem(jsonData, "data"); // JSON字段没有包含 data
    if (dataPayloadJson == NULL)
    {
        ESP_LOGE(TAG, "JSON Parse failed. Unable to obtain [ data ]");
        ESP_LOGE(TAG, "\n%.*s", mqttRecvData->dataLen, mqttRecvData->data);
        cJSON_Delete(jsonData);
        return ESP_FAIL;
    }
    for (size_t i = 0; i < MQTT_CONTROL_TYPE_CLASSIFY_MAX; i++)
    {
        if ((_mqttContorType >= s_sysSetPageHandle[i].minClassifyNum) && (_mqttContorType <= s_sysSetPageHandle[i].maxClassifyNum))
        {
            err = s_sysSetPageHandle[i].mqtt_cmd_handle(_mqttContorType, _mqttCmdType, dataPayloadJson);
            cJSON_Delete(jsonData);
            return err;
        }
    }
    cJSON_Delete(jsonData);
    return ESP_FAIL;
}

/**
 * @brief  从默认主题发布MQTT字符消息
 * @param  controlType  消息类型
 * @param  notifyType   通知类型
 * @param  jsonObjName  JSON元素名
 * @param  str          字符串内容
 */
void mqttDefaultTopicPubStrMsg(uint16_t controlType, uint16_t notifyType, const char *jsonObjName, const char *str)
{
    MqttPublishData_t _mqttPubData;
    cJSON *msgBuff = NULL;
    msgBuff = cJSON_CreateObject();
    cJSON_AddNumberToObject(msgBuff, "control_type", controlType);
    cJSON_AddNumberToObject(msgBuff, "notify_type", notifyType);
    cJSON *data = NULL;
    data = cJSON_CreateObject();
    cJSON_AddStringToObject(data, jsonObjName, str);
    cJSON_AddItemToObject(msgBuff, "data", data);
    char *jsonStr = NULL;
    jsonStr = cJSON_PrintUnformatted(msgBuff);
    _mqttPubData.dataLen = strlen(jsonStr);
    strcpy(_mqttPubData.data, jsonStr);
    xQueueSend(g_mqttPubDataQueueHandler, &_mqttPubData, pdMS_TO_TICKS(100));
    cJSON_free(jsonStr);
    cJSON_Delete(msgBuff);
}

/**
 * @brief  从默认主题发布MQTT数字消息
 * @param  controlType  消息类型
 * @param  notifyType   通知类型
 * @param  jsonObjName  JSON元素名
 * @param  num          数字内容
 */
void mqttDefaultTopicPubNumMsg(uint16_t controlType, uint16_t notifyType, const char *jsonObjName, uint32_t num)
{
    MqttPublishData_t _mqttPubData;
    cJSON *msgBuff = NULL;
    msgBuff = cJSON_CreateObject();
    cJSON_AddNumberToObject(msgBuff, "control_type", controlType);
    cJSON_AddNumberToObject(msgBuff, "notify_type", notifyType);
    cJSON *data = NULL;
    data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, jsonObjName, num);
    cJSON_AddItemToObject(msgBuff, "data", data);
    char *jsonStr = NULL;
    jsonStr = cJSON_PrintUnformatted(msgBuff);
    _mqttPubData.dataLen = strlen(jsonStr);
    strcpy(_mqttPubData.data, jsonStr);
    xQueueSend(g_mqttPubDataQueueHandler, &_mqttPubData, pdMS_TO_TICKS(100));
    cJSON_free(jsonStr);
    cJSON_Delete(msgBuff);
}

/**
 * @brief MQTT TASK
 * @param  pvParameters
 */
void mqttTask(void *pvParameters)
{
    MqttReceiveData_t mqttRecvData;
    MqttPublishData_t mqttPubData;
    static char pubTopic[MQTT_TOPIC_MAX_LEN] = {0};
    static int pubQos;
    pubQos = g_nvsData.networkConfigData.mqttConfigData.pubQos;
    strcpy(pubTopic, g_nvsData.networkConfigData.mqttConfigData.pubTopic);
    esp_err_t err;
    for (;;)
    {
        if (xQueueReceive(g_mqttRecvDataQueueHandler, &mqttRecvData, pdMS_TO_TICKS(10)) == pdTRUE)
        {

            err = mqttCmdRecvHandle(&mqttRecvData);
            ESP_ERROR_CHECK_WITHOUT_ABORT(err);
            if (err == ESP_OK && (getMqttState() == MQTT_READY)) // MQTT命令执行成功，将命令从另外的Topic回显
            {
                esp_mqtt_client_publish(g_mqttClientHandle, pubTopic, mqttRecvData.data, mqttRecvData.dataLen, pubQos, 0);
                // ESP_LOGI(TAG, "MQTT Publish. Topic: %.*s", strlen(pubTopic), pubTopic);
                // ESP_LOGI(TAG, "MQTT Publish Data:\n%.*s", mqttRecvData.dataLen, mqttRecvData.data);
            }
            else // 打印执行失败的命令
            {
                ESP_LOGW(TAG, "Failed command. Topic: %.*s", mqttRecvData.topicLen, mqttRecvData.topic);
                ESP_LOGW(TAG, "Failed command Data:\n%.*s", mqttRecvData.dataLen, mqttRecvData.data);
            }
        }
        if (getMqttState() == MQTT_READY)
        {
            if (xQueueReceive(g_mqttPubDataQueueHandler, &mqttPubData, pdMS_TO_TICKS(10)) == pdTRUE) // 处理队列中的发送任务
            {
                ESP_LOGI(TAG, "MQTT Publish. Topic: %.*s", strlen(pubTopic), pubTopic);
                ESP_LOGI(TAG, "MQTT Publish Data:\n%.*s", mqttPubData.dataLen, mqttPubData.data);
                esp_mqtt_client_publish(g_mqttClientHandle, pubTopic, mqttPubData.data, mqttPubData.dataLen, pubQos, 0);
            }
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    vTaskDelete(NULL);
}
