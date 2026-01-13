/**
 * @file ledStripIndicationTask.c
 * @brief 灯带指示逻辑处理
 * @version 1.0
 * @date 2024-04-16
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "user_tasks.h"
// 常量定义

#define RAW_DATA_MAX_POINTS 60                        // 定义触摸点的最大数量
#define RAW_DATA_MIN_PROCESS_POINTS 43                // 定义触摸点最小处理数量
#define CALCULATE_INTERVAL_TIME_MS 100                // 帧处理定时器
#define CALCULATE_INTERVAL_COLLECTED_TIMEOUT_COUNT 13 // 帧处理定时器超时次数（定时器N次触发，未收集满最小处理数量，清空缓存点）
#define INFRARED_TOUCH_POINT_SIZE 10                  // 每个触摸点数据的大小
#define INVALID_POINT_ID 0xFF                         // 无效的触摸点ID
#define HSV_BLUE_HUE 221                              // 蓝色色调值
#define HSV_BLUE_SATURATION 216                       // 蓝色饱和度
#define HSV_YELLOW_HUE 32                             // 黄色色调值
#define HSV_YELLOW_SATURATION 255                     // 黄色饱和度
#define WRONG_TAKE_LED_BLINK_TIMES 2                  // 拿错灯带闪烁次数
#define WARNING_BLINK_INTERVAL_MS 150                 // 警告闪烁间隔
#define WARNING_PAUSE_INTERVAL_MS 100                 // 警告暂停间隔
#define ORDER_DONE_TLED_BLINK_TIMES 2                 // 订单完成闪烁次数
#define ORDER_DONE_BLINK_INTERVAL_MS 450              // 订单完成闪烁间隔
#define ORDER_DONE_PAUSE_INTERVAL_MS 300              // 订单完成暂停间隔
#define ORDER_DONE_DELAY_INTERVAL_MS 900              // 订单完成延迟闪烁时间

EventGroupHandle_t g_ifTouchDataFLowEventGroup; // 触摸传感器数据流事件组
BoxParam_t g_drawBoxParam = {
    // 灯带库位参数，全局变量 仅在工程设置界面中访问使用
    .boxName = "",
    .maxX = 0,
    .minX = INFRARED_TOUCH_DATA_VALUE_MAXNUM,
    .maxY = 0,
    .minY = INFRARED_TOUCH_DATA_VALUE_MAXNUM,
    .beginLed = SCREEN_LEDSTRIP_BEGIN_LED_TEXT_DEFAULT_DATA,
    .endLed = SCREEN_LEDSTRIP_END_LED_TEXT_DEFAULT_DATA,
}; // 灯带库位参数

// 定义日志标签
static const char *TAG = "InfraredTouch";
static TimerHandle_t s_xFrameTimer;                       // 帧切片定时器
static QueueHandle_t s_checkedBoxesQueue;                 // 刚检测过的库位队列
static uint8_t s_checkedBoxesQueueLen = 1;                // 刚检测过的库位队列长度 （允许前N个再次拿取）
static uint8_t s_detectNum = 1;                           // 一次检测的库位数量
static uint16_t s_boxCount = 0;                           // NVS中存储的库位个数
static uint16_t s_validBoxCount = 0;                      // 本次拿取实际有效的库位计数
static BoxParam_t *s_boxParamList = NULL;                 // 库位参数列表指针
static BoxPointCount_t *s_boxPointCounts = NULL;          // 库位点数统计计算指针
static TouchPoint_t s_touchBuffer[RAW_DATA_MAX_POINTS];   // 存储触摸点数据
static size_t s_touchPointCount = 0;                      // 当前累计的触摸点数量
static SemaphoreHandle_t s_boxDataUpdateSemaphore = NULL; // 库位数据分类完毕二值信号量

/**
 * @brief 添加触摸点数据到缓冲区
 * @details 将新的触摸点坐标添加到缓冲区中,用于后续的点位统计
 *          缓冲区大小由RAW_DATA_MAX_POINTS定义
 *          每个触摸点包含X,Y坐标信息
 * @param x X坐标值 范围:0-INFRARED_TOUCH_DATA_VALUE_MAXNUM
 * @param y Y坐标值 范围:0-INFRARED_TOUCH_DATA_VALUE_MAXNUM
 * @return true 添加成功, false 缓冲区已满
 */
bool add_touch_point(int x, int y)
{
    if (s_touchPointCount < RAW_DATA_MAX_POINTS)
    {
        s_touchBuffer[s_touchPointCount].x = x;
        s_touchBuffer[s_touchPointCount].y = y;
        s_touchPointCount++;
        return true;
    }
    // ESP_DRAM_LOGW(TAG, "Touch buffer is full, point dropped: x=%d, y=%d", x, y);
    return false;
}

/**
 * @brief  检测触摸点的XY坐标最大最小值
 * @param  touchX
 * @param  touchY
 */
void process_touch_max_data(uint16_t touchX, uint16_t touchY)
{
    if (touchX >= g_drawBoxParam.maxX)
    {
        g_drawBoxParam.maxX = touchX;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_X_TEXT, touchX);
    }
    if (touchX < g_drawBoxParam.minX)
    {
        g_drawBoxParam.minX = touchX;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_X_TEXT, touchX);
    }
    if (touchY >= g_drawBoxParam.maxY)
    {
        g_drawBoxParam.maxY = touchY;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_Y_TEXT, touchY);
    }
    if (touchY < g_drawBoxParam.minY)
    {
        g_drawBoxParam.minY = touchY;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_Y_TEXT, touchY);
    }
}

/**
 * @brief 红外触摸传感器数据处理
 * @param data 传感器数据
 */
void infraredTouchReportHandel(const uint8_t *const data)
{
    if (data == NULL)
        return;

    EventBits_t uxBits = xEventGroupGetBits(g_ifTouchDataFLowEventGroup);
    uint16_t touchX = 0, touchY = 0;

    if (uxBits & TOUCH_MIN_MAX_MODE_BIT)
    {
        touchX = (data[4] << 8) | data[3];
        touchY = (data[6] << 8) | data[5];
        process_touch_max_data(touchX, touchY);
        return;
    }

    if (!(uxBits & TOUCH_CENTER_MODE_BIT))
        return;

    // 解析所有触摸点并添加到缓冲区
    for (uint8_t i = 0; i < INFRARED_TOUCH_DATA_FRAME_MAX_POINTS; i++)
    {
        uint8_t pointId = data[i * INFRARED_TOUCH_POINT_SIZE + 2];
        if (pointId == INVALID_POINT_ID)
            break;

        touchX = (data[i * INFRARED_TOUCH_POINT_SIZE + 4] << 8) | data[i * INFRARED_TOUCH_POINT_SIZE + 3];
        touchY = (data[i * INFRARED_TOUCH_POINT_SIZE + 6] << 8) | data[i * INFRARED_TOUCH_POINT_SIZE + 5];
        // esp_rom_printf("Point[%d] X: %d, Y: %d \n\r", pointId, touchX, touchY);

        add_touch_point(touchX, touchY);
    }
}

/**
 * @brief 帧计算定时器回调函数
 * @details 每隔CALCULATE_INTERVAL_TIME_MS时间处理一次累积的触摸点数据:
 *          1. 检查触摸点数量是否达到最小处理阈值(RAW_DATA_MIN_PROCESS_POINTS)
 *          2. 将每个触摸点分类到对应的库位区域,通过比对XY坐标是否在库位范围内
 *          3. 统计每个库位区域的触摸点数量,包括未分类区域(unclassified)
 *          4. 按触摸点数量从大到小排序,排序结果用于判断实际操作的库位
 *          5. 处理完成后通过信号量通知主任务
 * @param xTimer 定时器句柄
 */
void vFrameTimerCallback(TimerHandle_t xTimer)
{
    static uint8_t rawDataDiscardCount = 0;
    if (s_touchPointCount < RAW_DATA_MIN_PROCESS_POINTS)
    {
        rawDataDiscardCount++;
        if (rawDataDiscardCount >= CALCULATE_INTERVAL_COLLECTED_TIMEOUT_COUNT)
        {
            s_touchPointCount = 0;
            rawDataDiscardCount = 0;
            // ESP_LOGE(TAG, "点数不足 抛弃数据");
        }
        return;
    }

    if (s_boxCount == 0) // NVS中未存储库位信息（一般为新硬件初始化阶段）
    {
        // 重置触摸点计数
        s_touchPointCount = 0;
        return;
    }

    // 重置计数数组
    memset(s_boxPointCounts, 0, (s_boxCount + 1) * sizeof(BoxPointCount_t));
    s_validBoxCount = 0;

    // 初始化 unclassified (作为第一个元素)
    strncpy(s_boxPointCounts[0].boxName, "unclassified",
            LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1);
    s_boxPointCounts[0].boxName[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1] = '\0';
    s_boxPointCounts[0].pointCount = 0;
    s_validBoxCount = 1; // unclassified 占用第一个位置

    // 处理每个触摸点
    for (size_t i = 0; i < s_touchPointCount; i++)
    {
        bool pointClassified = false;
        uint16_t touchX = s_touchBuffer[i].x;
        uint16_t touchY = s_touchBuffer[i].y;
        // 检查点是否在任何一个库位的范围内
        for (size_t j = 0; j < s_boxCount; j++)
        {
            // 先检查X坐标是否在范围内
            if (touchX >= s_boxParamList[j].minX && touchX <= s_boxParamList[j].maxX)
            {
                // 只有在X坐标满足的情况下，才检查Y坐标
                if (touchY >= s_boxParamList[j].minY && touchY <= s_boxParamList[j].maxY)
                {
                    // 查找或创建对应的计数记录
                    bool found = false;
                    for (size_t k = 1; k < s_validBoxCount; k++) // 从1开始，跳过unclassified
                    {
                        if (strcmp(s_boxPointCounts[k].boxName, s_boxParamList[j].boxName) == 0)
                        {
                            s_boxPointCounts[k].pointCount++;
                            found = true;
                            pointClassified = true;
                            break;
                        }
                    }

                    if (!found && s_validBoxCount < s_boxCount + 1) // +1 为unclassified预留空间
                    {
                        strncpy(s_boxPointCounts[s_validBoxCount].boxName,
                                s_boxParamList[j].boxName,
                                LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1);
                        s_boxPointCounts[s_validBoxCount].boxName[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1] = '\0';
                        s_boxPointCounts[s_validBoxCount].pointCount = 1;
                        s_validBoxCount++;
                        pointClassified = true;
                    }
                    break;
                }
            }
        }

        // 如果点不在任何库位内，计入 unclassified
        if (!pointClassified)
        {
            s_boxPointCounts[0].pointCount++; // unclassified 在索引0的位置
        }
    }

    // 对统计结果进行排序（包含 unclassified）
    for (size_t i = 0; i < s_validBoxCount + 1; i++) // +1 包含 unclassified
    {
        for (size_t j = i + 1; j < s_validBoxCount + 1; j++) // +1 包含 unclassified
        {
            if (s_boxPointCounts[i].pointCount < s_boxPointCounts[j].pointCount)
            {
                // 交换两个元素
                BoxPointCount_t temp = s_boxPointCounts[i];
                s_boxPointCounts[i] = s_boxPointCounts[j];
                s_boxPointCounts[j] = temp;
            }
        }
    }

    ESP_LOGI(TAG, "Box touch point statistics:");
    for (size_t i = 0; i < s_validBoxCount + 1; i++)
    {
        if (s_boxPointCounts[i].pointCount > 0) // 只打印点数大于0的库位
        {
            ESP_LOGI(TAG, "Box[%s]: %u points", s_boxPointCounts[i].boxName, s_boxPointCounts[i].pointCount);
        }
    }

    // 重置触摸点计数
    s_touchPointCount = 0;

    // 数据更新完成，释放信号量
    xSemaphoreGive(s_boxDataUpdateSemaphore);
}

/**
 * @brief 初始化NVS存储的库位列表
 * @details 从NVS读取并解析库位参数配置:
 *          1. 从NVS读取JSON格式的库位参数配置
 *          2. 解析JSON获取库位总数,分配必要的内存
 *          3. 解析每个库位的参数:
 *             - 库位名称(唯一标识)
 *             - XY坐标范围(minX,maxX,minY,maxY)
 *             - LED灯带范围(beginLed,endLed)
 *          4. 初始化库位点数统计数组(包含unclassified)
 * @return ESP_OK 初始化成功
 * @return ESP_ERR_INVALID_STATE NVS中无配置数据
 * @return ESP_ERR_INVALID_ARG JSON解析失败
 * @return ESP_ERR_NO_MEM 内存分配失败
 */
esp_err_t initializeBoxList()
{
    // 读取NVS中库位的JSON字符串
    char *boxParamStr = readBoxParamFromNvs();
    if (boxParamStr == NULL)
    {
        ESP_LOGE(TAG, "Failed to read box param from NVS.");
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG, "BoxParam: %s", boxParamStr);

    cJSON *storedBoxParamJSON = cJSON_Parse(boxParamStr);
    heap_caps_free(boxParamStr);
    if (storedBoxParamJSON == NULL)
    {
        ESP_LOGE(TAG, "Failed to parse stored box param JSON.");
        return ESP_ERR_INVALID_ARG;
    }

    // 根据JSON键值对的数量确定存储的库位数量
    s_boxCount = cJSON_GetArraySize(storedBoxParamJSON);
    if (s_boxCount == 0) // 库位数量为0 为不允许出现的情况。  没有库位应当擦除KEY 为 NVS_BOX_PARAM_DATA 的NVS存储
    {
        cJSON_Delete(storedBoxParamJSON);
        // 该任务无法继续，进入错误状态
        while (1)
        {
            ESP_LOGE(TAG, "No box param data found in storedBoxParamJSON.");
            vTaskDelay(pdMS_TO_TICKS(5000));
            setScreenPage(SCREEN_MESSAGE_DIALOG_PAGE);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "找不到有效库位数据，请排查存储。", "Unable to find valid storage location data, please check machine storage.", "有効なライブラリビットデータが見つかりません。マシンストレージを調べてください。");
            vTaskDelay(pdMS_TO_TICKS(15000));
        }
    }
    ESP_LOGI(TAG, "Box count: %d", s_boxCount);

    // 分配库位参数内存
    s_boxParamList = (BoxParam_t *)heap_caps_malloc(s_boxCount * sizeof(BoxParam_t), MALLOC_CAP_SPIRAM);
    if (s_boxParamList == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for s_boxParamList");
        cJSON_Delete(storedBoxParamJSON);
        return ESP_ERR_NO_MEM;
    }
    // 分配库位点数计算内存
    s_boxPointCounts = (BoxPointCount_t *)heap_caps_malloc((s_boxCount + 1) * sizeof(BoxPointCount_t), MALLOC_CAP_SPIRAM); // +1 for unclassified
    if (s_boxPointCounts == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for s_boxPointCounts");
        heap_caps_free(s_boxParamList);
        s_boxParamList = NULL;
        cJSON_Delete(storedBoxParamJSON);
        return ESP_ERR_NO_MEM;
    }

    // 遍历JSON对象中的每个键值对
    cJSON *boxItem = NULL;
    int currentIndex = 0;
    cJSON_ArrayForEach(boxItem, storedBoxParamJSON)
    {

        // 获取库位名称（键）
        const char *boxName = boxItem->string;

        // 获取坐标数组（值）
        if (cJSON_IsArray(boxItem) && cJSON_GetArraySize(boxItem) == 6)
        {
            // 复制库位名称
            strncpy(s_boxParamList[currentIndex].boxName, boxName,
                    LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1);
            s_boxParamList[currentIndex].boxName[LED_STRIP_INDICATION_STORAGE_LOCATION_STR_MAXSIZE - 1] = '\0';

            // 获取坐标值
            s_boxParamList[currentIndex].minX = cJSON_GetArrayItem(boxItem, 0)->valueint;
            s_boxParamList[currentIndex].maxX = cJSON_GetArrayItem(boxItem, 1)->valueint;
            s_boxParamList[currentIndex].minY = cJSON_GetArrayItem(boxItem, 2)->valueint;
            s_boxParamList[currentIndex].maxY = cJSON_GetArrayItem(boxItem, 3)->valueint;
            s_boxParamList[currentIndex].beginLed = cJSON_GetArrayItem(boxItem, 4)->valueint;
            s_boxParamList[currentIndex].endLed = cJSON_GetArrayItem(boxItem, 5)->valueint;
            currentIndex++;
        }
    }

    cJSON_Delete(storedBoxParamJSON);
    return ESP_OK;
}

/**
 * 初始化定时器
 */
esp_err_t initTimer()
{
    s_xFrameTimer = xTimerCreate(
        (const char *)"FrameTimer",                // 定时器名称
        pdMS_TO_TICKS(CALCULATE_INTERVAL_TIME_MS), // 定时器周期
        pdTRUE,                                    // 重载定时器
        (void *)0,                                 // 定时器ID
        vFrameTimerCallback                        // 定时器回调函数
    );
    if (s_xFrameTimer == NULL)
    {
        // 创建定时器失败
        ESP_LOGE(TAG, "Failed to create the watchdog timer.\n");
        return ESP_FAIL;
    }
    if (xTimerStart(s_xFrameTimer, 0) != pdPASS)
    {
        // 重置定时器失败
        ESP_LOGE(TAG, "Failed to Start the watchdog timer.");
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief 设置指定范围LED的颜色
 * @param startLed 起始LED
 * @param endLed 结束LED
 * @param r 红色分量
 * @param g 绿色分量
 * @param b 蓝色分量
 */
static void setLedRangeColor(uint16_t startLed, uint16_t endLed, uint8_t r, uint8_t g, uint8_t b)
{
    for (uint16_t i = startLed; i <= endLed; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, i - 1, r, g, b);
    }
}

/**
 * @brief 设置指定范围LED的HSV颜色
 * @param startLed 起始LED
 * @param endLed 结束LED
 * @param h 色调
 * @param s 饱和度
 * @param v 亮度
 */
static void setLedRangeColorHSV(uint16_t startLed, uint16_t endLed, uint16_t h, uint16_t s, uint8_t v)
{
    for (uint16_t i = startLed; i <= endLed; i++)
    {
        led_strip_set_pixel_hsv(g_ledstripRmtHandle, i - 1, h, s, v);
    }
}

/**
 * @brief 关闭库位灯光
 * @param lightOffBox 需要关闭的库位
 */
static void setBoxLightColorOff(const BoxParam_t *lightOffBox)
{
    if (lightOffBox == NULL)
        return;
    setLedRangeColor(lightOffBox->beginLed, lightOffBox->endLed, 0, 0, 0);
}

/**
 * @brief 设置库位的灯珠颜色
 * @param lightOnBox 需要点亮的库位
 * @param brightness 亮度
 * @param color 颜色
 */
static void setBoxLightColorOn(const BoxParam_t *lightOnBox, uint8_t brightness, LedStripColor color)
{
    if (lightOnBox == NULL)
        return;

    switch (color)
    {
    case LED_STRIP_RED:
        setLedRangeColor(lightOnBox->beginLed, lightOnBox->endLed, brightness, 0, 0);
        break;
    case LED_STRIP_GREEN:
        setLedRangeColor(lightOnBox->beginLed, lightOnBox->endLed, 0, brightness, 0);
        break;
    case LED_STRIP_BLUE:
        setLedRangeColorHSV(lightOnBox->beginLed, lightOnBox->endLed, HSV_BLUE_HUE, HSV_BLUE_SATURATION, brightness);
        break;
    case LED_STRIP_YELLOW:
        setLedRangeColorHSV(lightOnBox->beginLed, lightOnBox->endLed, HSV_YELLOW_HUE, HSV_YELLOW_SATURATION, brightness);
        break;
    default:
        break;
    }
}

/**
 * @brief  全部完成灯带绿色闪烁
 * @param  brightness
 */
static void orderDoneBlink(uint8_t brightness)
{
    static uint16_t ledNum = 0;
    if (ledNum == 0)
    {
        ledNum = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
    }
    vTaskDelay(pdMS_TO_TICKS(ORDER_DONE_DELAY_INTERVAL_MS));
    for (size_t n = 0; n < ORDER_DONE_TLED_BLINK_TIMES; n++)
    {
        // 全绿
        for (size_t i = 1; i <= ledNum; i++)
        {
            led_strip_set_pixel(g_ledstripRmtHandle, i - 1, 0, brightness, 0);
        }
        LEDSTRIP_REFRESH;
        vTaskDelay(pdMS_TO_TICKS(ORDER_DONE_BLINK_INTERVAL_MS));

        // 全灭
        for (size_t i = 1; i <= ledNum; i++)
        {
            led_strip_set_pixel(g_ledstripRmtHandle, i - 1, 0, 0, 0);
        }
        LEDSTRIP_REFRESH;
        if (n < ORDER_DONE_TLED_BLINK_TIMES)
        {
            vTaskDelay(pdMS_TO_TICKS(ORDER_DONE_PAUSE_INTERVAL_MS));
        }
    }
}

/**
 * @brief  拿错灯光警告
 * @param  checkingBoxes
 * @param  brightness
 */
static void mismatchWaring(BoxParam_t *checkingBoxes, uint8_t brightness)
{
    static uint16_t ledNum = 0;
    if (ledNum == 0)
    {
        ledNum = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
    }
    for (size_t n = 0; n < WRONG_TAKE_LED_BLINK_TIMES; n++)
    {
        // 全红
        for (size_t i = 1; i <= ledNum; i++)
        {
            led_strip_set_pixel(g_ledstripRmtHandle, i - 1, brightness, 0, 0);
        }
        // 目标库位设绿色
        for (size_t i = 0; i < s_detectNum; i++)
        {
            if (checkingBoxes[i].boxName[0] != '\0')
            {
                setBoxLightColorOn(&checkingBoxes[i], brightness, LED_STRIP_GREEN);
            }
        }
        // 上一个库位设黄色
        UBaseType_t queueSize = uxQueueMessagesWaiting(s_checkedBoxesQueue);
        for (size_t i = 0; i < queueSize; i++)
        {
            BoxParam_t checkedBox;
            xQueueReceive(s_checkedBoxesQueue, &checkedBox, 0);
            setBoxLightColorOn(&checkedBox, brightness, LED_STRIP_YELLOW); // 黄灯
            LEDSTRIP_REFRESH;
            // 重新插入
            xQueueSendToBack(s_checkedBoxesQueue, &checkedBox, 0);
        }
        LEDSTRIP_REFRESH;
        vTaskDelay(pdMS_TO_TICKS(WARNING_BLINK_INTERVAL_MS));

        // 全灭
        for (size_t i = 1; i <= ledNum; i++)
        {
            led_strip_set_pixel(g_ledstripRmtHandle, i - 1, 0, 0, 0);
        }
        // 目标库位设绿色
        for (size_t i = 0; i < s_detectNum; i++)
        {
            if (checkingBoxes[i].boxName[0] != '\0')
            {
                setBoxLightColorOn(&checkingBoxes[i], brightness, LED_STRIP_GREEN);
            }
        }
        // 上一个库位设黄色
        for (size_t i = 0; i < queueSize; i++)
        {
            BoxParam_t checkedBox;
            xQueueReceive(s_checkedBoxesQueue, &checkedBox, 0);
            setBoxLightColorOn(&checkedBox, brightness, LED_STRIP_YELLOW); // 黄灯
            LEDSTRIP_REFRESH;
            // 重新插入
            xQueueSendToBack(s_checkedBoxesQueue, &checkedBox, 0);
        }
        LEDSTRIP_REFRESH;
        if (n < WRONG_TAKE_LED_BLINK_TIMES)
        {
            vTaskDelay(pdMS_TO_TICKS(WARNING_PAUSE_INTERVAL_MS));
        }
    }
}

/**
 * @brief 灯带指示任务主函数
 * @details 实现工作流程:
 *          1. 初始化:
 *             - 从NVS加载库位参数配置
 *             - 初始化定时器用于触摸点处理
 *             - 创建信号量和队列
 *          2. 订单处理循环:
 *             - 等待MQTT订单数据
 *             - 按顺序处理订单中的库位
 *             - 通过灯带颜色指示要拿取的库位(绿色)
 *             - 通过触摸检测验证拿取正确性
 *             - 正确拿取后转为黄色指示
 *             - 拿错发出警告闪烁
 * @param pvParameters 任务参数(未使用)
 */
void ledStripIndicationTask(void *pvParameters)
{
    ESP_ERROR_CHECK_WITHOUT_ABORT(initializeBoxList());
    initTimer();

    // 创建二值信号量
    s_boxDataUpdateSemaphore = xSemaphoreCreateBinary();
    if (s_boxDataUpdateSemaphore == NULL)
    {
        ESP_LOGE(TAG, "Failed to create box data update semaphore");
        return;
    }

    // 初始化检测库位数组
    BoxParam_t checkingBoxes[s_detectNum];
    // 初始化已经检测的队列
    s_checkedBoxesQueue = xQueueCreate(s_checkedBoxesQueueLen, sizeof(BoxParam_t));
    while (1)
    {
        // 等待MQTT订单到来
        xSemaphoreTake(g_ledStripNewOrderSemphHandle, portMAX_DELAY);
        ESP_LOGI(TAG, "--------------Start processing new order--------------");
        UBaseType_t _orderQueueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler); // 队列中待检测的库位数量
        uint16_t brightness = g_nvsData.DeviceConfigData.ledstripConfigData.btightness;

        // 初始化为空字符串表示未使用
        for (int i = 0; i < s_detectNum; i++)
        {
            checkingBoxes[i].boxName[0] = '\0';
        }
        xQueueReset(s_checkedBoxesQueue);
        // 重置触摸点计数
        s_touchPointCount = 0;
        // 重置计数数组
        memset(s_boxPointCounts, 0, (s_boxCount + 1) * sizeof(BoxPointCount_t));
        s_validBoxCount = 0;
        // 尝试取走剩余信号量
        xSemaphoreTake(s_boxDataUpdateSemaphore, 0);
        bool isMatchDetection = pdTRUE; // 检测是否匹配
        uint16_t _errorCount = 0;       // 步骤错误次数统计
        uint16_t _seqNo = 0;            // 订单步骤汇报计数
        // 内部自行判断订单完成，break结束死循环
        while (1)
        {
            // 检测匹配 获取新的待检测库位，只填充空位置
            if (isMatchDetection)
            {
                // 初始化步骤不报告
                if (_seqNo != 0)
                {
                    ESP_LOGI(TAG, "Step [%d] pickup completed, error count is [%d]", _seqNo, _errorCount);
                    // mqttcPubStepDoneMsg(_seqNo, _errorCount); // 离线版本不汇报步骤完成
                }
                for (int i = 0; i < s_detectNum; i++)
                {
                    if (checkingBoxes[i].boxName[0] == '\0' && _orderQueueLen > 0)
                    {
                        BoxParam_t addBox;
                        bool isAllowToAdd;
                        xQueueReceive(g_ledStripBoxDataQueueHandler, &addBox, 0);
                        _orderQueueLen--;
                        do
                        {
                            isAllowToAdd = pdTRUE; // 假设可以添加库位
                            for (size_t j = 0; j < s_detectNum; j++)
                            {
                                // 取出来的库位和检测中的名字重复了
                                if (strcmp(checkingBoxes[j].boxName, addBox.boxName) == 0)
                                {
                                    isAllowToAdd = pdFALSE; // 假设不成立，不可添加
                                    break;
                                }
                            }
                            if (!isAllowToAdd && _orderQueueLen > 0)
                            {
                                xQueueReceive(g_ledStripBoxDataQueueHandler, &addBox, 0);
                                _orderQueueLen--;
                            }
                            else
                            {
                                break;
                            }
                        } while (!isAllowToAdd);

                        if (isAllowToAdd)
                        {
                            checkingBoxes[i] = addBox;
                            _seqNo++;
                        }
                    }
                }
                _errorCount = 0;
                isMatchDetection = pdFALSE;
            }

            ESP_LOGI(TAG, "The box currently being tested:");

            bool isOrderDone = pdTRUE;
            for (int i = 0; i < s_detectNum; i++)
            {
                if (checkingBoxes[i].boxName[0] != '\0')
                {
                    ESP_LOGI(TAG, "[%d]: %s", i, checkingBoxes[i].boxName);
                    setBoxLightColorOn(&checkingBoxes[i], brightness, LED_STRIP_GREEN);
                    LEDSTRIP_REFRESH;
                    isOrderDone = pdFALSE;
                }
            }
            if (isOrderDone)
            {
                break;
            }

            // 阻塞等待触摸数据更新完成，期间需要判断订单是否被终止
            bool isOrderCancel = pdFALSE;
            while (!xSemaphoreTake(s_boxDataUpdateSemaphore, pdMS_TO_TICKS(10)))
            {
                if (xSemaphoreTake(g_ledStripCancelOrderSemphHandle, 0)) // 收到订单结束信号
                {
                    isOrderCancel = pdTRUE;
                    LEDSTRIP_CLEAR;
                    ESP_LOGI(TAG, "--------------order cancel--------------");
                    break;
                }
            }
            if (isOrderCancel)
            {
                break;
            }

            // 排名第一的是未分类的点
            if (strcmp(s_boxPointCounts[0].boxName, "unclassified") == 0)
            {
                ESP_LOGE(TAG, "Touch detected in invalid area, triggering warning");
                isMatchDetection = pdFALSE;
                mismatchWaring(checkingBoxes, brightness);
                // 尝试取走警告期间产生的信号量
                xSemaphoreTake(s_boxDataUpdateSemaphore, 0);
            }
            else // 检查每个库位是否与前N名匹配
            {
                for (int i = 0; i < s_detectNum; i++)
                {

                    // 跳过空位置
                    if (checkingBoxes[i].boxName[0] == '\0')
                    {
                        continue;
                    }

                    for (int j = 0; j < s_validBoxCount && j < s_detectNum; j++)
                    {
                        if (strcmp(checkingBoxes[i].boxName, s_boxPointCounts[j].boxName) == 0)
                        {
                            ESP_LOGI(TAG, "Box[%s] picked correctly", checkingBoxes[i].boxName);
                            // 转换为黄灯
                            setBoxLightColorOn(&checkingBoxes[i], brightness, LED_STRIP_YELLOW);
                            LEDSTRIP_REFRESH;
                            // 尝试将已检测库位插入队列，如果队列已满，则丢弃最旧的元素
                            if (xQueueSend(s_checkedBoxesQueue, &checkingBoxes[i], 0) != pdPASS)
                            {
                                // 队列已满，丢弃最旧的元素
                                BoxParam_t discardedBox;
                                xQueueReceive(s_checkedBoxesQueue, &discardedBox, 0); // 丢弃最旧的元素
                                setBoxLightColorOff(&discardedBox);                   // 关闭黄灯
                                LEDSTRIP_REFRESH;
                                // 重新插入新的库位
                                xQueueSend(s_checkedBoxesQueue, &checkingBoxes[i], 0);
                            }
                            // 检测通过后清空该位置
                            checkingBoxes[i].boxName[0] = '\0';
                            isMatchDetection = pdTRUE;
                            break;
                        }
                    }
                }
                // 拿取位置不匹配
                if (!isMatchDetection)
                {
                    BoxParam_t tempBox;
                    UBaseType_t queueSize = uxQueueMessagesWaiting(s_checkedBoxesQueue);
                    bool isPreviousBox = pdFALSE;
                    for (size_t i = 0; i < queueSize; i++)
                    {
                        xQueueReceive(s_checkedBoxesQueue, &tempBox, 0);
                        for (int j = 0; j < s_validBoxCount && j < s_detectNum; j++)
                        {
                            if (strcmp(tempBox.boxName, s_boxPointCounts[j].boxName) == 0)
                            {
                                isPreviousBox = pdTRUE;
                            }
                        }
                        xQueueSendToBack(s_checkedBoxesQueue, &tempBox, 0); // 放回队列
                    }
                    if (isPreviousBox)
                    {
                        ESP_LOGW(TAG, "Retrieve the previous box again");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Not detected this time, issue an alarm!");
                        _errorCount++;
                        for (size_t j = 0; j < s_boxCount; j++)
                        {
                            if (strcmp(s_boxPointCounts[0].boxName, s_boxParamList[j].boxName) == 0)
                            {
                                mismatchWaring(checkingBoxes, brightness);
                                // 尝试取走警告期间产生的信号量
                                xSemaphoreTake(s_boxDataUpdateSemaphore, 0);
                                break;
                            }
                        }
                    }
                }
            }
        }
        orderDoneBlink(brightness);
        ESP_LOGI(TAG, "--------------End order processing--------------");
    }
    // 清理资源
    vSemaphoreDelete(s_boxDataUpdateSemaphore);
    if (s_boxParamList != NULL)
    {
        heap_caps_free(s_boxParamList);
        s_boxParamList = NULL;
    }
    if (s_boxPointCounts != NULL)
    {
        heap_caps_free(s_boxPointCounts);
        s_boxPointCounts = NULL;
    }
    vTaskDelete(NULL);
}
