/**
 * @file system_type .c
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
QueueHandle_t g_ledStripBoxDataQueueHandler; // 灯带指示物料，物料位置信息
SemaphoreHandle_t g_ledStripBoxDataSemphHandle;
/**
 * @brief 订单残留库位状态记录结构体
 */
typedef struct _OrderState
{
    uint16_t residueBoxCount;                               // 订单剩余未取货库位
    uint32_t color;                                         // 订单的库位颜色
    char orderName[LED_STRIP_INDICATION_ORDER_STR_MAXSIZE]; // 订单名称
    uint64_t timeStamp;                                     // 订单提交时间戳
    uint16_t orderNo;                                       // 订单到达的先后顺序
} OrderState_t;

static OrderState_t s_orderState[LED_STRIP_INDICATION_MAX_ORDERS] = {0};
static uint8_t s_executingOrderCount = 0; // 系统执行中的订单计数
static uint16_t s_orderNoIncremental = 0; // 递增的订单数（系统开机直至目前执行过的订单数）

static bool s_ledstripEnabled;                           // 灯带的使能状态
static uint16_t s_initLednum;                            // 初始化的灯珠数量
static uint8_t s_btightness;                             // 初始化的亮度
static bool s_allowOrderOverwriteLocation;               // 是否允许库位覆盖
char _orderName[LED_STRIP_INDICATION_ORDER_STR_MAXSIZE]; // 订单名称

/**
 * @brief 同步订单三色灯数据
 */
static void alarmLedSync(void)
{
    static bool initialized = false;
    static uint32_t _ledStripIndicationColorGreen, _ledStripIndicationColorYellow, _ledStripIndicationColorRed, _ledStripIndicationColorBlue;
    if (!initialized)
    {
        _ledStripIndicationColorGreen = g_nvsData.projectConfigData.ledStripIndicationConfigData.colorGreen;
        _ledStripIndicationColorYellow = g_nvsData.projectConfigData.ledStripIndicationConfigData.colorYellow;
        _ledStripIndicationColorRed = g_nvsData.projectConfigData.ledStripIndicationConfigData.colorRed;
        _ledStripIndicationColorBlue = g_nvsData.projectConfigData.ledStripIndicationConfigData.colorBlue;
        initialized = true;
    }
    uint8_t alarmLedState = 0x00;
    for (size_t n = 0; n < LED_STRIP_INDICATION_MAX_ORDERS; n++)
    {
        if (s_orderState[n].residueBoxCount > 0 && s_orderState[n].color == _ledStripIndicationColorGreen)
        {
            alarmLedState = alarmLedState | 0x01;
        }
        if (s_orderState[n].residueBoxCount > 0 && s_orderState[n].color == _ledStripIndicationColorYellow)
        {
            alarmLedState = alarmLedState | 0x02;
        }
        if (s_orderState[n].residueBoxCount > 0 && s_orderState[n].color == _ledStripIndicationColorRed)
        {
            alarmLedState = alarmLedState | 0x04;
        }
        if (s_orderState[n].residueBoxCount > 0 && s_orderState[n].color == _ledStripIndicationColorBlue)
        {
            alarmLedState = alarmLedState | 0x08;
        }
    }
    alarmLedStateSet(alarmLedState);
}

/**
 * @brief  处理下发新订单命令
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripPlaceNewOrder(cJSON *data)
{
    cJSON *_timeStampJson = getJSONobj(data, "time_stamp");
    cJSON *_colorJson = getJSONobj(data, "color");
    cJSON *_orderJson = getJSONobj(data, "order");
    cJSON *_boxListJson = getJSONobj(data, "box_list");
    if (_timeStampJson == NULL || _colorJson == NULL || _boxListJson == NULL || _orderJson == NULL)
    {
        return ESP_FAIL;
    }
    OrderBoxInfo_t _orderBoxInfo = {0}; // 当前命令的订单信息
    strcpy(_orderName, cJSON_GetStringValue(_orderJson));
    uint64_t _timeStamp = 0;
    _timeStamp = cJSON_GetNumberValue(_timeStampJson);
    _orderBoxInfo.color = cJSON_GetNumberValue(_colorJson);

    int boxListJsonSize = cJSON_GetArraySize(_boxListJson);
    if (boxListJsonSize <= 0) // 库位列表是空的
    {
        ESP_LOGE(TAG, "The storage location is empty");
        return ESP_ERR_INVALID_ARG;
    }
    if (cJSON_GetArraySize(cJSON_GetArrayItem(_boxListJson, 0)) != LED_STRIP_INDICATION_BOX_LIST_ITEM_SIZE) // 库位信息长度不一致
    {
        ESP_LOGE(TAG, "Storage location information error");
        return ESP_ERR_INVALID_ARG;
    }

    bool _isOrderExist = false;
    for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++) // 查找当前订单是否已经存在
    {
        if (strcmp(_orderName, s_orderState[i].orderName) == 0)
        {
            _isOrderExist = true;
            if (!s_allowOrderOverwriteLocation) // 是否允许覆盖库位
            {
                ESP_LOGE(TAG, "The system does not allow orders to overwrite storage locations");
                ESP_LOGE(TAG, "The order [%s] still has residual [%d] storage locations", s_orderState[i].orderName, s_orderState[i].residueBoxCount);
                return ESP_ERR_INVALID_STATE;
            }
        }
    }

    if (!_isOrderExist) // 订单不存在，尝试添加新订单
    {
        if (s_executingOrderCount >= LED_STRIP_INDICATION_MAX_ORDERS) // 订单已经排满了
        {
            ESP_LOGE(TAG, "s_executingOrderCount = %d Exceeding the number of orders", s_executingOrderCount);
            return ESP_ERR_INVALID_SIZE;
        }
        else
        {
            for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
            {
                if (s_orderState[i].orderNo == 0) // 找到空闲的位置
                {
                    s_executingOrderCount++;            // 执行中的订单加1
                    if (s_orderNoIncremental == 0xFFFF) // 累计订单超过uint16_t 归零计数
                    {
                        s_orderNoIncremental = 0;
                    }
                    s_orderNoIncremental++;
                    s_orderState[i].orderNo = s_orderNoIncremental; // 赋予订单到达顺序
                    _orderBoxInfo.orderNo = s_orderNoIncremental;
                    strcpy(s_orderState[i].orderName, _orderName);
                    s_orderState[i].timeStamp = _timeStamp;
                    s_orderState[i].color = _orderBoxInfo.color;
                    ESP_LOGW(TAG, "s_orderState add a new order [%s],order No = [%d]", s_orderState[i].orderName, s_orderState[i].orderNo);
                    break;
                }
            }
        }
    }
    char _StorageLocationStr[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE]; // 库位索引
    uint16_t _startLedId = 0;
    uint16_t _endledId = 0;
    uint16_t _takeTimes = 0;
    BoxData_t _boxdataTemp = {0};

    for (size_t i = 0; i < boxListJsonSize; i++) // 挨个取出库位数据，存入队列
    {
        strcpy(_StorageLocationStr, cJSON_GetStringValue(cJSON_GetArrayItem(cJSON_GetArrayItem(_boxListJson, i), 0))); // 获取库位
        _startLedId = cJSON_GetNumberValue(cJSON_GetArrayItem(cJSON_GetArrayItem(_boxListJson, i), 1));                // 获取起始灯珠
        _endledId = cJSON_GetNumberValue(cJSON_GetArrayItem(cJSON_GetArrayItem(_boxListJson, i), 2));                  // 获取结尾灯珠
        _takeTimes = cJSON_GetNumberValue(cJSON_GetArrayItem(cJSON_GetArrayItem(_boxListJson, i), 3));                 // 获取灭灯寿命
        _orderBoxInfo.takeTimes = _takeTimes;                                                                          // 获得到订单的每一项
        strcpy(_boxdataTemp.storageLocation, _StorageLocationStr);
        _boxdataTemp.startLedId = _startLedId;
        _boxdataTemp.endLedId = _endledId;
        ESP_LOGI(TAG, "Get Order BoxList[%d] = [\"%s\",%d,%d,%d]", i, _StorageLocationStr, _startLedId, _endledId, _takeTimes);
        if (_endledId < _startLedId || (_endledId - _startLedId + 1) < LED_STRIP_INDICATION_MAX_ORDERS || _endledId > s_initLednum) // 错误库位灯珠数据过滤
        {
            ESP_LOGE(TAG, "The number of leds in BoxList[%d] is incorrect", i);
            // 清除未出错之前已经塞入的库位的数据,无法使用计数倒回查找,因为插入的算法已经将队列打散

            UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
            if (_queueLen == 0) // 队列空的 直接删除订单,无需删除库位
            {
                goto DELETE_ORDER;
            }

            for (size_t j = 0; j < _queueLen; j++) // 遍历链表查找已有元素
            {
                BoxData_t _boxDataDelete;
                xQueueReceive(g_ledStripBoxDataQueueHandler, &_boxDataDelete, 0);
                for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++)
                {
                    if (_boxDataDelete.orderBoxInfo[k].orderNo == s_orderNoIncremental)
                    {
                        memset(&_boxDataDelete.orderBoxInfo[k], 0, sizeof(OrderBoxInfo_t)); // 该库位清空订单占用数据
                        bool isAllOrderDone = true;                                         // 所有订单取货完成
                        for (size_t x = 0; x < LED_STRIP_INDICATION_MAX_ORDERS; x++)
                        {
                            if (_boxDataDelete.orderBoxInfo[x].orderNo != 0)
                            {
                                isAllOrderDone = false;
                            }
                        }
                        if (isAllOrderDone) // 该库位的订单已经全部取货完成,数据不放回队列
                        {
                            ESP_LOGW(TAG, "Box[\"%s\"] Delete", _boxDataDelete.storageLocation);
                        }
                        else // 还有其他订单未完成,数据放回队列
                        {
                            ESP_LOGW(TAG, "Box[\"%s\"] Delete order [%s] ", _boxDataDelete.storageLocation, _orderName);
                            xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_boxDataDelete, 0);
                        }
                        break;
                    }
                    if (k == LED_STRIP_INDICATION_MAX_ORDERS - 1) // 订单中没有匹配的库位，放回队列
                    {
                        xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_boxDataDelete, 0);
                    }
                }
            }
        DELETE_ORDER:
            // 清除订单状态
            for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
            {
                if (s_orderState[j].orderNo == s_orderNoIncremental) // 删除订单状态
                {
                    memset(&s_orderState[j], 0, sizeof(OrderState_t));
                    s_executingOrderCount--;
                    ESP_LOGE(TAG, "Order [%s] data incorrect. Delete! [%d] orders remaining", _orderName, s_executingOrderCount);
                    break;
                }
            }
            return ESP_ERR_INVALID_ARG;
        }

        // printf("Storage Location = %s\nStart LED ID = %d\nEnd LED ID = %d\nTake Times = %d\n", _StorageLocationStr, _startLedId, _endledId, _takeTimes);
        UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
        if (_queueLen == 0) // 队列空的 塞入第一个库位
        {
            ESP_LOGI(TAG, "Box List is empty. Insert Fist Box [%s]", _StorageLocationStr);
            for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
            {
                if (s_orderState[j].orderNo == _orderBoxInfo.orderNo)
                {
                    s_orderState[j].residueBoxCount++;
                }
            }
            _boxdataTemp.isBoxOrderChanged = true;
            memcpy(&_boxdataTemp.orderBoxInfo[0], &_orderBoxInfo, sizeof(OrderBoxInfo_t)); // 第一个订单占用库位
            xQueueSend(g_ledStripBoxDataQueueHandler, &_boxdataTemp, 0);
        }
        else //  队列非空  寻找库位元素是否已经存在，存在则合并，不存在则写入
        {
            BoxData_t _searchBoxData = {0};        // 查找的目标数据
            for (size_t j = 0; j < _queueLen; j++) // 遍历链表查找已有元素
            {
                // 从队列中接收元素
                if (xQueueReceive(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0) == pdPASS)
                {
                    // ESP_LOGI(TAG, "_searchBoxData.storageLocation = %s", _searchBoxData.storageLocation);
                    if (strcmp(_boxdataTemp.storageLocation, _searchBoxData.storageLocation) == 0) // 找到相等的库位
                    {
                        // ESP_LOGI(TAG, "Matching storage location found");
                        bool _isBoxOrderChanged = false;
                        for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++) // 先查找该库位相同的订单，相同则写入更新
                        {
                            if (_searchBoxData.orderBoxInfo[k].orderNo == _orderBoxInfo.orderNo)
                            {
                                ESP_LOGE(TAG, "Order[%s] Order No = [%d] Box[%s] info changed", _orderName, _orderBoxInfo.orderNo, _boxdataTemp.storageLocation);
                                _isBoxOrderChanged = true;
                                // 订单的库位发生覆盖,需要把旧的(当前亮着的)库位熄灭 (会影响所有订单的库位)
                                for (size_t x = _searchBoxData.startLedId; x <= _searchBoxData.endLedId; x++)
                                {
                                    led_strip_set_pixel(g_ledstripRmtHandle, x - 1, 0, 0, 0);
                                }
                                // 重新赋值库位数据
                                _searchBoxData.startLedId = _boxdataTemp.startLedId;
                                _searchBoxData.endLedId = _boxdataTemp.endLedId;
                                _searchBoxData.isBoxOrderChanged = _isBoxOrderChanged;
                                memcpy(&_searchBoxData.orderBoxInfo[k], &_orderBoxInfo, sizeof(OrderBoxInfo_t));
                                xQueueSend(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                                break;
                            }
                        }
                        if (!_isBoxOrderChanged) // 无相同订单再查找该库位是否有剩余订单空间
                        {
                            for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++)
                            {
                                if (_searchBoxData.orderBoxInfo[k].orderNo == 0)
                                {
                                    ESP_LOGI(TAG, "Box[%s] Add a new order [%s]", _boxdataTemp.storageLocation, _orderName);
                                    for (size_t x = 0; x < LED_STRIP_INDICATION_MAX_ORDERS; x++)
                                    {
                                        if (s_orderState[x].orderNo == _orderBoxInfo.orderNo)
                                        {
                                            s_orderState[x].residueBoxCount++;
                                        }
                                    }
                                    _isBoxOrderChanged = true;
                                    // 订单的库位发生覆盖,需要把旧的(当前亮着的)库位熄灭 (会影响所有订单的库位)
                                    for (size_t x = _searchBoxData.startLedId; x <= _searchBoxData.endLedId; x++)
                                    {
                                        led_strip_set_pixel(g_ledstripRmtHandle, x - 1, 0, 0, 0);
                                    }
                                    // 重新赋值库位数据
                                    _searchBoxData.startLedId = _boxdataTemp.startLedId;
                                    _searchBoxData.endLedId = _boxdataTemp.endLedId;
                                    _searchBoxData.isBoxOrderChanged = _isBoxOrderChanged;
                                    memcpy(&_searchBoxData.orderBoxInfo[k], &_orderBoxInfo, sizeof(OrderBoxInfo_t));
                                    xQueueSend(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                                    break;
                                }
                            }
                        }
                        if (_isBoxOrderChanged)
                        {
                            goto FIND_EQUAL;
                        }
                        else // 库位中已占用的订单不匹配，且库位没剩余的订单空间。
                        {
                            ESP_LOGE(TAG, "The number of orders exceeds the limit");
                            return ESP_ERR_INVALID_SIZE;
                        }
                    }
                    else
                    {
                        // 元素不符合条件，放回队列尾部 继续查找
                        xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                    }
                }
            }
            // 遍历完成未找到相等的库位，库位写入队列
            // ESP_LOGI(TAG, "The Box [%s] is new. Add to the queue", _boxdataTemp.storageLocation);
            for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
            {
                if (s_orderState[j].orderNo == _orderBoxInfo.orderNo)
                {
                    s_orderState[j].residueBoxCount++;
                }
            }
            _boxdataTemp.isBoxOrderChanged = true;
            memcpy(&_boxdataTemp.orderBoxInfo[0], &_orderBoxInfo, sizeof(OrderBoxInfo_t)); // 占用第一个订单位置
            xQueueSend(g_ledStripBoxDataQueueHandler, &_boxdataTemp, 0);
        FIND_EQUAL:
        }
    }
    alarmLedSync();
    xSemaphoreGive(g_ledStripBoxDataSemphHandle); // 库位数据变化,释放信号量处理亮灯
    return ESP_OK;
}

/**
 * @brief  查询残留订单
 * @return esp_err_t
 */
esp_err_t queryResiduesOrder()
{
    MqttPublishData_t _mqttPubData;
    cJSON *msgBuff = NULL;
    msgBuff = cJSON_CreateObject();
    cJSON_AddNumberToObject(msgBuff, "control_type", MQTT_CONTROL_TYPE_BUSINESS_ORDER_MANAGE);
    cJSON_AddNumberToObject(msgBuff, "notify_type", NOTIFY_RESIDUES_ORDER);
    cJSON *data = NULL;
    data = cJSON_CreateObject();
    cJSON *orderList = NULL;
    orderList = cJSON_CreateArray();
    cJSON *orderInfoArray = NULL;

    for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
    {
        orderInfoArray = cJSON_CreateArray();
        if (s_orderState[i].residueBoxCount > 0)
        {

            cJSON_AddItemToArray(orderInfoArray, cJSON_CreateString(s_orderState[i].orderName));
            cJSON_AddItemToArray(orderInfoArray, cJSON_CreateNumber(s_orderState[i].residueBoxCount));
            cJSON_AddItemToArray(orderInfoArray, cJSON_CreateNumber(s_orderState[i].timeStamp));
            cJSON_AddItemToArray(orderInfoArray, cJSON_CreateNumber(s_orderState[i].color));
            cJSON_AddItemToArray(orderList, orderInfoArray);
        }
    }
    cJSON_AddItemToObject(data, "order_list", orderList);
    cJSON_AddItemToObject(msgBuff, "data", data);
    char *jsonStr = NULL;
    jsonStr = cJSON_PrintUnformatted(msgBuff);
    _mqttPubData.dataLen = strlen(jsonStr);
    strcpy(_mqttPubData.data, jsonStr);
    xQueueSend(g_mqttPubDataQueueHandler, &_mqttPubData, pdMS_TO_TICKS(100));
    cJSON_free(jsonStr);
    cJSON_Delete(msgBuff);
    return ESP_OK;
}

/**
 * @brief  取货完成
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripPickupCompleted(cJSON *data)
{
    cJSON *_orderJson = getJSONobj(data, "order");
    cJSON *_boxJson = getJSONobj(data, "box");
    cJSON *_times = getJSONobj(data, "times");
    if (_boxJson == NULL || _orderJson == NULL || _times == NULL)
    {
        return ESP_FAIL;
    }
    strcpy(_orderName, cJSON_GetStringValue(_orderJson));
    uint16_t _orderNoForSerch = 0; // 用来搜索的订单顺序
    bool _orderEffective = false;  // 当前订单有效
    for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
    {
        if (strcmp(_orderName, s_orderState[i].orderName) == 0)
        {
            _orderNoForSerch = s_orderState[i].orderNo;
            _orderEffective = true;
        }
    }

    char _storageLocation[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE] = {0};
    strcpy(_storageLocation, cJSON_GetStringValue(_boxJson));
    uint16_t deductTakeTimes = cJSON_GetNumberValue(_times); // 本次扣除的取物次数
    UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
    if (_queueLen == 0 || !_orderEffective) // 订单全部完成,或该订单无效
    {
        ESP_LOGE(TAG, "Order [%s] Order No = [%d] have been completed", _orderName, _orderNoForSerch);
        return ESP_FAIL;
    }
    BoxData_t _searchBoxData = {0};        // 查找的目标数据
    for (size_t i = 0; i < _queueLen; i++) // 遍历链表查找已有元素
    {
        // 从队列中接收元素
        if (xQueueReceive(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0) == pdPASS)
        {
            // ESP_LOGI(TAG, "_searchBoxData.storageLocation = %s", _searchBoxData.storageLocation);
            if (strcmp(_storageLocation, _searchBoxData.storageLocation) == 0) // 找到相等的库位
            {
                // ESP_LOGI(TAG, "Matching storage location found");
                for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++) // 查找该库位的订单，相同则更新取货次数数据
                {
                    if (_searchBoxData.orderBoxInfo[j].orderNo == _orderNoForSerch)
                    {
                        if (_searchBoxData.orderBoxInfo[j].takeTimes > deductTakeTimes) // 该订单还有剩余取货次数，扣减取货寿命
                        {
                            _searchBoxData.orderBoxInfo[j].takeTimes -= deductTakeTimes;
                            ESP_LOGI(TAG, "Order [%s] Box [%s] changed.take times deduct [%d] residue [%d]", _orderName, _storageLocation, deductTakeTimes, _searchBoxData.orderBoxInfo[j].takeTimes);
                            xQueueSend(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                            return ESP_OK;
                            break;
                        }
                        else // 订单的该库位取货完成,执行灭灯,订单数据归零
                        {
                            ESP_LOGI(TAG, "Order [%s] Box [%s] kill", _orderName, _storageLocation);
                            for (size_t k = _searchBoxData.orderBoxInfo[j].ownerStartLedId; k <= _searchBoxData.orderBoxInfo[j].ownerEndLedId; k++)
                            {
                                led_strip_set_pixel(g_ledstripRmtHandle, k - 1, 0, 0, 0);
                            }
                            led_strip_refresh(g_ledstripRmtHandle);
                            for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++) // 订单残留指示减1个库位
                            {
                                if (s_orderState[k].orderNo == _searchBoxData.orderBoxInfo[j].orderNo && s_orderState[k].residueBoxCount >= 1)
                                {
                                    s_orderState[k].residueBoxCount--;
                                    if (s_orderState[k].residueBoxCount == 0) // 订单的所有库位取货完毕了,订单已经完成，订单数据归零
                                    {
                                        memset(&s_orderState[k], 0, sizeof(OrderState_t));
                                        s_executingOrderCount--;
                                        ESP_LOGW(TAG, "Order [%s] completed. [%d] orders remaining", _orderName, s_executingOrderCount);
                                        alarmLedSync();
                                    }
                                }
                            }
                            memset(&_searchBoxData.orderBoxInfo[j], 0, sizeof(OrderBoxInfo_t)); // 该库位清空订单占用数据
                            bool isAllOrderDone = true;                                         // 所有订单取货完成
                            for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++)
                            {
                                if (_searchBoxData.orderBoxInfo[k].orderNo != 0)
                                {
                                    isAllOrderDone = false;
                                }
                            }
                            if (isAllOrderDone) // 该库位的订单已经全部取货完成,数据不放回队列
                            {
                                return ESP_OK;
                                break;
                            }
                            else // 还有其他订单未完成,数据放回队列
                            {
                                xQueueSend(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                                return ESP_OK;
                                break;
                            }
                        }
                    }
                }
                // 库位匹配但找不到目标订单,库位放回队列尾部
                ESP_LOGE(TAG, "Box [%s],Not have order [%s] No. = [%d] exists", _storageLocation, _orderName, _orderNoForSerch);
                xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
                return ESP_ERR_NOT_FOUND;
            }
            else
            {
                // 库位不符合条件，放回队列尾部 继续查找
                xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
            }
        }
    }
    // 队列中没有该库位
    ESP_LOGE(TAG, "Box [%s],Not in queue", _storageLocation);
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief  结束取货指示
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripEndPickupInstruction(cJSON *data)
{
    cJSON *_orderJson = getJSONobj(data, "order");
    if (_orderJson == NULL)
    {
        return ESP_FAIL;
    }
    strcpy(_orderName, cJSON_GetStringValue(_orderJson));
    uint16_t _orderNoForSerch = 0;
    bool _orderEffective = false; // 当前订单有效
    for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
    {
        if (strcmp(_orderName, s_orderState[i].orderName) == 0)
        {
            _orderNoForSerch = s_orderState[i].orderNo;
            _orderEffective = true;
        }
    }
    UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
    if (_queueLen == 0 || !_orderEffective) // 订单全部完成,或该订单无效
    {
        ESP_LOGE(TAG, "Order [%s] has been completed", _orderName);
        return ESP_FAIL;
    }
    BoxData_t _searchBoxData = {0};        // 查找的目标数据
    for (size_t i = 0; i < _queueLen; i++) // 遍历链表查找已有元素,熄灭灯带
    {
        // 从队列中接收元素,清除相等订单的库位灯带占用信息
        if (xQueueReceive(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0) == pdPASS)
        {
            for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
            {
                if (_orderNoForSerch == _searchBoxData.orderBoxInfo[j].orderNo) // 找到相等的订单
                {
                    for (size_t k = _searchBoxData.orderBoxInfo[j].ownerStartLedId; k <= _searchBoxData.orderBoxInfo[j].ownerEndLedId; k++)
                    {
                        led_strip_set_pixel(g_ledstripRmtHandle, k - 1, 0, 0, 0);
                    }
                    led_strip_refresh(g_ledstripRmtHandle);
                    memset(&_searchBoxData.orderBoxInfo[j], 0, sizeof(OrderBoxInfo_t)); // 清空该库位
                }
            }
            bool isAllOrderDone = true;
            for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
            {
                if (_searchBoxData.orderBoxInfo[j].orderNo != 0)
                {
                    isAllOrderDone = false;
                }
            }
            if (isAllOrderDone) // 所有订单已经取货完成,该库位数据不放回队列
            {
            }
            else
            {
                xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_searchBoxData, 0);
            }
        }
        // 清除三色灯状态
        for (size_t j = 0; j < LED_STRIP_INDICATION_MAX_ORDERS; j++)
        {
            if (_orderNoForSerch == s_orderState[j].orderNo) // 找到相等的订单
            {
                memset(&s_orderState[j], 0, sizeof(OrderState_t));
                s_executingOrderCount--;
                ESP_LOGW(TAG, "Order [%s] kill. [%d] orders remaining", _orderName, s_executingOrderCount);
                alarmLedSync();
            }
        }
    }
    return ESP_OK;
}

/**
 * @brief  灯珠定位
 * @param  data
 * @return esp_err_t
 */
esp_err_t ledStripLocate(cJSON *data)
{
    cJSON *_startLedJson = getJSONobj(data, "start_led");
    cJSON *_endLedJson = getJSONobj(data, "end_led");
    if (_startLedJson == NULL || _endLedJson == NULL)
    {
        return ESP_FAIL;
    }
    uint16_t _startLed = cJSON_GetNumberValue(_startLedJson);
    uint16_t _endLed = cJSON_GetNumberValue(_endLedJson);
    UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
    if (_queueLen != 0)
    {
        xQueueReset(g_ledStripBoxDataQueueHandler);
        s_executingOrderCount = 0;
        for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
        {
            memset(&s_orderState[i], 0, sizeof(OrderState_t));
        }
        alarmLedSync();
        ESP_LOGW(TAG, "led Locate debug. All Order kill.");
    }
    led_strip_clear(g_ledstripRmtHandle);
    for (size_t i = _startLed; i <= _endLed; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, i - 1, s_btightness, s_btightness, s_btightness);
    }
    led_strip_refresh(g_ledstripRmtHandle);
    return ESP_OK;
}

/**
 * @brief  灯珠定位超时显示重置
 * @param  xTimer
 */
void vWatchdogCallback(TimerHandle_t xTimer)
{
    // 超时后执行的代码
    led_strip_clear(g_ledstripRmtHandle);
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
    if (_startLedJson == NULL || _endLedJson == NULL)
    {
        return ESP_FAIL;
    }
    uint16_t _startLed = cJSON_GetNumberValue(_startLedJson);
    uint16_t _endLed = cJSON_GetNumberValue(_endLedJson);
    uint32_t _color = cJSON_GetNumberValue(_colorJson);
    uint8_t _red = ((_color >> 16) & 0XFF) / 20;
    uint8_t _green = ((_color >> 8) & 0XFF) / 20;
    uint8_t _blue = (_color & 0XFF) / 20;

    UBaseType_t _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
    if (_queueLen != 0)
    {
        xQueueReset(g_ledStripBoxDataQueueHandler);
        s_executingOrderCount = 0;
        for (size_t i = 0; i < LED_STRIP_INDICATION_MAX_ORDERS; i++)
        {
            memset(&s_orderState[i], 0, sizeof(OrderState_t));
        }
        alarmLedSync();
        ESP_LOGW(TAG, "led sequence debug. All Order kill.");
    }
    led_strip_clear(g_ledstripRmtHandle);
    for (size_t i = _startLed; i <= _endLed; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, i - 1, _red, _green, _blue);
        led_strip_refresh(g_ledstripRmtHandle);
    }
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
    static bool ledstripDebugState = false; // 灯带处于灯珠定位的状态
    static TimerHandle_t xWatchdogTimer;    // 灯珠定位显示超时定时器
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
    // --------------------------------------------------- 下发新订单 --------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_ORDER_MANAGE:
        if ((mqttCmdType == PLACE_NEW_ORDER || mqttCmdType == PLACE_NEW_ORDER_BY_NODE_RED) && s_ledstripEnabled)
        {
            if (ledstripDebugState) // 退出灯带调试模式
            {
                led_strip_clear(g_ledstripRmtHandle);
                if (xTimerDelete(xWatchdogTimer, 0) != pdPASS)
                {
                    // 重置定时器失败
                    ESP_LOGE(TAG, "Failed to Delete the watchdog timer.\n");
                }
                ledstripDebugState = false;
            }
            return ledStripPlaceNewOrder(data);
        }
        else if (mqttCmdType == QUERY_RESIDUES_ORDER)
        {
            return queryResiduesOrder();
        }
        else
        {
            ESP_LOGE(TAG, "mqttCmdType = [%d], Command not supported", mqttCmdType);
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;

    // ---------------------------------------------------取物完成命令--------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_PICKUP_COMPLETED:
        if (mqttCmdType == PICKUP_COMPLETED && s_ledstripEnabled)
        {
            if (ledstripDebugState) // 退出灯带调试模式
            {
                led_strip_clear(g_ledstripRmtHandle);
                if (xTimerDelete(xWatchdogTimer, 0) != pdPASS)
                {
                    // 重置定时器失败
                    ESP_LOGE(TAG, "Failed to Delete the watchdog timer.\n");
                }
                ledstripDebugState = false;
            }
            return ledStripPickupCompleted(data);
        }
        else
        {
            ESP_LOGE(TAG, "mqttCmdType = [%d], Command not supported", mqttCmdType);
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;

    // ---------------------------------------------------结束指示灭灯--------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_END_PICKUP_INSTRUCTION:
        if ((mqttCmdType == END_PICKUP_INSTRUCTION || mqttCmdType == END_PICKUP_INSTRUCTION_BY_NODE_RED) && s_ledstripEnabled)
        {
            if (ledstripDebugState) // 退出灯带调试模式
            {
                led_strip_clear(g_ledstripRmtHandle);
                if (xTimerDelete(xWatchdogTimer, 0) != pdPASS)
                {
                    // 重置定时器失败
                    ESP_LOGE(TAG, "Failed to Delete the watchdog timer.\n");
                }
                ledstripDebugState = false;
            }
            return ledStripEndPickupInstruction(data);
        }
        else
        {
            ESP_LOGE(TAG, "mqttCmdType = [%d], Command not supported", mqttCmdType);
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;
    // ---------------------------------------------------灯带调试--------------------------------------------------------------------
    case MQTT_CONTROL_TYPE_BUSINESS_LEDSTRIP_DEBUG:
        if (mqttCmdType == LED_LOCATE && s_ledstripEnabled) // 灯珠定位
        {
            if (ledstripDebugState)
            {
                // 重置看门狗定时器
                if (xTimerReset(xWatchdogTimer, 0) != pdPASS)
                {
                    // 重置定时器失败
                    ESP_LOGE(TAG, "Failed to reset the watchdog timer.");
                }
            }
            else
            {
                ledstripDebugState = true;
                xWatchdogTimer = xTimerCreate(
                    (const char *)"WatchdogTimer",                          // 定时器名称
                    pdMS_TO_TICKS(LED_STRIP_INDICATION_LED_LOCATE_TIMEOUT), // 定时器周期
                    pdFALSE,                                                // 不重载一次性定时器
                    (void *)0,                                              // 定时器ID
                    vWatchdogCallback                                       // 定时器回调函数
                );

                if (xWatchdogTimer != NULL)
                {
                    // 启动定时器
                    if (xTimerStart(xWatchdogTimer, 0) != pdPASS)
                    {
                        // 启动定时器失败
                        ESP_LOGI(TAG, "Failed to start the watchdog timer.\n");
                    }
                }
                else
                {
                    // 创建定时器失败
                    ESP_LOGI(TAG, "Failed to create the watchdog timer.\n");
                }
            }
            return ledStripLocate(data);
        }
        else if (mqttCmdType == LED_SEQUENCE && s_ledstripEnabled) // 灯珠顺序跑马
        {
            return ledSequence(data);
        }
        else
        {
            ESP_LOGE(TAG, "mqttCmdType = [%d], Command not supported", mqttCmdType);
            return ESP_ERR_NOT_SUPPORTED;
        }
        break;
    default:
        ESP_LOGE(TAG, "mqttContorType = [%d], Command not supported", mqttContorType);
        return ESP_ERR_NOT_SUPPORTED;
        break;
    }
    return ESP_FAIL;
}
