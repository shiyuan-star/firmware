/**
 * @file business_type .c
 * @brief MQTT操作业务指令
 * @version 1.0
 * @date 2024-04-07
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "network.h"
#include "business.h"

static char *TAG = "MQTT_BUSSINESS_CMD";
QueueHandle_t g_ledStripBoxDataQueueHandler;        // 灯带指示物料，物料位置信息
SemaphoreHandle_t g_ledStripNewOrderSemphHandle;    // 收到新的指示订单信号亮
SemaphoreHandle_t g_ledStripCancelOrderSemphHandle; // 取消指示订单信号亮
OrderInfo_t g_orderInfo = {0};
static bool s_ledstripEnabled;             // 灯带的使能状态
static uint16_t s_initLednum;              // 初始化的灯珠数量
static uint8_t s_btightness;               // 初始化的亮度
static bool s_allowOrderOverwriteLocation; // 是否允许库位覆盖

/**
 * @brief  处理下发新订单命令
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripPlaceNewOrder(cJSON *data)
{
    cJSON *_boxListJson = getJSONobj(data, "box_list");
    cJSON *_timeStampJson = getJSONobj(data, "time_stamp");
    cJSON *_orderCountJson = getJSONobj(data, "order_count");
    cJSON *_orderNameJson = getJSONobj(data, "order_name");
    if (_timeStampJson == NULL || _orderCountJson == NULL || _boxListJson == NULL || _orderNameJson == NULL)
    {
        return ESP_FAIL;
    }
    strcpy(g_orderInfo.orderName, cJSON_GetStringValue(_orderNameJson));
    g_orderInfo.timeStamp = cJSON_GetNumberValue(_timeStampJson);
    g_orderInfo.orderCount = cJSON_GetNumberValue(_orderCountJson);
    // 打印订单信息
    ESP_LOGI(TAG, "Order Name: %s, Order Count: %d, Time Stamp: %lld", g_orderInfo.orderName, g_orderInfo.orderCount, g_orderInfo.timeStamp);

    BoxParam_t _cmdBoxParam = {0}; // 当前命令的订单信息
    // 获取库位物料盒LIST长度
    uint16_t _boxListSize = cJSON_GetArraySize(_boxListJson);
    char *_boxName = NULL;

    // 读取NVS中库位的JSON字符串
    char *boxParamStr = readBoxParamFromNvs();
    if (boxParamStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to read box param from NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    // ESP_LOGI(TAG, "BoxParam: %s", boxParamStr);

    cJSON *storedBoxParamJSON = cJSON_Parse(boxParamStr);
    heap_caps_free(boxParamStr);
    if (storedBoxParamJSON == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse stored box param JSON.");
        return ESP_ERR_INVALID_ARG;
    }

    xQueueReset(g_ledStripBoxDataQueueHandler); // 清空队列
    for (size_t i = 0; i < _boxListSize; i++)
    {
        _boxName = cJSON_GetStringValue(cJSON_GetArrayItem(_boxListJson, i));
        cJSON *existBoxParamDataArray = cJSON_GetObjectItem(storedBoxParamJSON, _boxName); // 从JSON中获取库位信息
        if (existBoxParamDataArray == NULL)
        {
            ESP_LOGE(TAG, "ledStripPlaceNewOrder: Failed to read Box Name: %s param from NVS.", _boxName);
            cJSON_Delete(storedBoxParamJSON);
            xQueueReset(g_ledStripBoxDataQueueHandler);
            return ESP_ERR_NOT_FOUND;
        }
        _cmdBoxParam.minX = cJSON_GetArrayItem(existBoxParamDataArray, 0)->valueint;
        _cmdBoxParam.maxX = cJSON_GetArrayItem(existBoxParamDataArray, 1)->valueint;
        _cmdBoxParam.minY = cJSON_GetArrayItem(existBoxParamDataArray, 2)->valueint;
        _cmdBoxParam.maxY = cJSON_GetArrayItem(existBoxParamDataArray, 3)->valueint;
        _cmdBoxParam.beginLed = cJSON_GetArrayItem(existBoxParamDataArray, 4)->valueint;
        _cmdBoxParam.endLed = cJSON_GetArrayItem(existBoxParamDataArray, 5)->valueint;
        strcpy(_cmdBoxParam.boxName, _boxName);
        // 打印库位信息
        ESP_LOGI(TAG, "Box Name: %s, minX: %d, maxX: %d, minY: %d, maxY: %d, beginLed: %d, endLed: %d", _cmdBoxParam.boxName, _cmdBoxParam.minX, _cmdBoxParam.maxX, _cmdBoxParam.minY, _cmdBoxParam.maxY, _cmdBoxParam.beginLed, _cmdBoxParam.endLed);
        xQueueSend(g_ledStripBoxDataQueueHandler, &_cmdBoxParam, pdMS_TO_TICKS(100));
    }
    xSemaphoreGive(g_ledStripNewOrderSemphHandle); // 库位数据变化,释放信号量处理亮灯
    // 尝试取走取消订单信号量
    xSemaphoreTake(g_ledStripCancelOrderSemphHandle, 0);
    cJSON_Delete(storedBoxParamJSON);
    return ESP_OK;
}

/**
 * @brief  处理删除订单
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripCancelOrder(cJSON *data)
{
    xQueueReset(g_ledStripBoxDataQueueHandler);       // 清空队列
    xSemaphoreGive(g_ledStripCancelOrderSemphHandle); // 取消订单信号量
    return ESP_OK;
}

/**
 * @brief  MQTT命令处理新增/修改 库位信息
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttModifyBoxInfo(cJSON *data)
{
    if (data == NULL)
    {
        return ESP_FAIL;
    }
    else
    {
        return mqttModifyBoxInfoSaveToNvs(data);
    }
}

/**
 * @brief  处理交互点亮库位
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripBoxLightUp(cJSON *data)
{
    cJSON *_boxListJson = getJSONobj(data, "box_list");
    cJSON *_colorJson = getJSONobj(data, "color");
    cJSON *_brightnessJson = getJSONobj(data, "brightness");
    if (_boxListJson == NULL || _colorJson == NULL || _brightnessJson == NULL)
    {
        return ESP_FAIL;
    }
    uint32_t _color = cJSON_GetNumberValue(_colorJson);
    uint16_t _brightness = cJSON_GetNumberValue(_brightnessJson);
    // 获取库位物料盒LIST长度
    uint16_t _boxListSize = cJSON_GetArraySize(_boxListJson);

    if (_boxListSize == 0) // 无物料盒，熄灭所有库位
    {
        LEDSTRIP_CLEAR;
        LEDSTRIP_REFRESH;
        return ESP_OK;
    }

    char *_boxName = NULL;

    // 读取NVS中库位的JSON字符串
    char *boxParamStr = readBoxParamFromNvs();
    if (boxParamStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to read box param from NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    // ESP_LOGI(TAG, "BoxParam: %s", boxParamStr);

    cJSON *storedBoxParamJSON = cJSON_Parse(boxParamStr);
    heap_caps_free(boxParamStr);
    if (storedBoxParamJSON == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse stored box param JSON.");
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t _red = ((_color >> 16) & 0XFF);
    uint8_t _green = ((_color >> 8) & 0XFF);
    uint8_t _blue = (_color & 0XFF);
    float hue;          // hue (0 - 360)
    uint8_t saturation; // saturation (0 - 255)
    uint8_t value;      // value (0 - 255)
    rgb8882HSV(_red, _green, _blue, &hue, &saturation, &value);
    value = _brightness;
    LEDSTRIP_CLEAR;
    for (size_t i = 0; i < _boxListSize; i++)
    {
        _boxName = cJSON_GetStringValue(cJSON_GetArrayItem(_boxListJson, i));
        cJSON *existBoxParamDataArray = cJSON_GetObjectItem(storedBoxParamJSON, _boxName); // 从JSON中获取库位信息
        if (existBoxParamDataArray == NULL)
        {
            ESP_LOGE(TAG, "ledStripBoxLightUp: Failed to read Box Name: %s param from NVS.", _boxName);
            cJSON_Delete(storedBoxParamJSON);
            return ESP_ERR_NOT_FOUND;
        }
        uint16_t beginLed = cJSON_GetArrayItem(existBoxParamDataArray, 4)->valueint;
        uint16_t endLed = cJSON_GetArrayItem(existBoxParamDataArray, 5)->valueint;
        for (size_t i = beginLed; i <= endLed; i++)
        {
            led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
        }
    }
    LEDSTRIP_REFRESH;
    cJSON_Delete(storedBoxParamJSON);
    return ESP_OK;
}

/**
 * @brief  灯珠顺序跑马
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledSequence(cJSON *data)
{
    cJSON *_startLedJson = getJSONobj(data, "start_led");
    cJSON *_endLedJson = getJSONobj(data, "end_led");
    cJSON *_colorJson = getJSONobj(data, "color");
    cJSON *_brightnessJson = getJSONobj(data, "brightness");
    cJSON *_delayJson = getJSONobj(data, "delay");
    if (_startLedJson == NULL || _endLedJson == NULL || _colorJson == NULL || _brightnessJson == NULL || _delayJson == NULL)
    {
        return ESP_FAIL;
    }
    uint16_t _startLed = cJSON_GetNumberValue(_startLedJson);
    uint16_t _endLed = cJSON_GetNumberValue(_endLedJson);
    uint32_t _color = cJSON_GetNumberValue(_colorJson);
    uint16_t _brightness = cJSON_GetNumberValue(_brightnessJson);
    uint32_t _delay = cJSON_GetNumberValue(_delayJson);
    uint8_t _red = ((_color >> 16) & 0XFF);
    uint8_t _green = ((_color >> 8) & 0XFF);
    uint8_t _blue = (_color & 0XFF);
    float hue;          // hue (0 - 360)
    uint8_t saturation; // saturation (0 - 255)
    uint8_t value;      // value (0 - 255)
    rgb8882HSV(_red, _green, _blue, &hue, &saturation, &value);
    value = _brightness;
    LEDSTRIP_CLEAR;
    if (_delay == 0)
    {
        for (size_t i = _startLed; i <= _endLed; i++)
        {
            led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
            LEDSTRIP_REFRESH;
        }
    }
    else
    {
        for (size_t i = _startLed; i <= _endLed; i++)
        {
            led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, hue, saturation, value);
            LEDSTRIP_REFRESH;
            vTaskDelay(pdMS_TO_TICKS(_delay));
        }
    }
    LEDSTRIP_CLEAR;
    return ESP_OK;
}

/**
 * @brief   MQTT操作业务指令处理函数
 * @param  mqttContorType
 * @param  mqttCmdType
 * @param  data
 * @return esp_err_t
 */
esp_err_t mqttSetBusinessHandle(uint16_t mqttContorType, uint16_t mqttCmdType, cJSON *data)
{
    static bool initialized = false;
    if (!initialized)
    {
        s_allowOrderOverwriteLocation = g_nvsData.projectConfigData.ledStripIndicationConfigData.allowOrderOverwriteLocation;
        s_ledstripEnabled = g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled;
        s_btightness = g_nvsData.DeviceConfigData.ledstripConfigData.btightness;
        s_initLednum = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
        initialized = true;
    }
    switch (mqttContorType)
    {
    // --------------------------------------------------- 订单相关 --------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_ORDER:
        if (mqttCmdType == PLACE_NEW_ORDER && s_ledstripEnabled)
        {
            return ledStripPlaceNewOrder(data); // 下发新订单
        }
        else if (mqttCmdType == CANCEL_ORDER && s_ledstripEnabled)
        {
            return ledStripCancelOrder(data);
        }
        break;
    // --------------------------------------------------- 库位相关 --------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_BOX_OPERATE:
        if (mqttCmdType == QUERY_BOX_INFO)
        {
            return mqttPubBoxInfoMsg();
        }
        else if (mqttCmdType == MODIFY_BOX_INFO)
        {
            return mqttModifyBoxInfo(data);
        }
        else if (mqttCmdType == DELETE_BOX_INFO)
        {
            return mqttDeleteBoxInfoSaveToNvs(data);
        }
        else if (mqttCmdType == DELETE_ALL_BOX_INFO)
        {
            return deleteAllBoxParamFromNvs();
        }
        else if (mqttCmdType == LIGHT_UP_BOX)
        {
            return ledStripBoxLightUp(data);
        }
        break;
    // --------------------------------------------------- 灯带操作相关 --------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_LEDSTRIP_OPERATE:
        if (mqttCmdType == LED_SEQUENCE_RUN)
        {
            return ledSequence(data);
        }
        break;

    default:
        ESP_LOGE(TAG, "mqttContorType = [%d], Command not supported", mqttContorType);
        return ESP_ERR_NOT_SUPPORTED;
        break;
    }

    ESP_LOGE(TAG, "mqttCmdType = [%d], Command not supported", mqttCmdType);
    return ESP_ERR_NOT_SUPPORTED;
}
