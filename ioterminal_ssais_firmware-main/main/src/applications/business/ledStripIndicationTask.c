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

#define FIRST_COME_FIRST_SERVED_UNLIMITED_LEDS_MODE 1 // 亮灯模式1: 从库位首部亮灯先到先得,不限订单对单个库位的灯珠占用数量
#define ORDER_MAXIMUM_LEDS_LIMIT_MODE 2               // 亮灯模式2: 从库位首部亮灯先到先得,限制灯珠

static char *TAG = "LEDSTRIP_INDICATION";

/**
 * @brief  颜色转换
 * @param  r
 * @param  g
 * @param  b
 * @param  hue
 * @param  saturation
 * @param  value
 */
void RGB8882HSV(unsigned char r, unsigned char g, unsigned char b, float *hue, unsigned char *saturation, unsigned char *value)
{
    float min, max, delta;

    // Convert RGB to [0,1]
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    min = rf < gf ? (rf < bf ? rf : bf) : (gf < bf ? gf : bf);
    max = rf > gf ? (rf > bf ? rf : bf) : (gf > bf ? gf : bf);

    *value = (unsigned char)(max * 255); // v

    delta = max - min;

    if (max != 0)
    {
        *saturation = (unsigned char)((delta / max) * 255); // s
    }
    else
    {
        // r = g = b = 0
        *saturation = 0;
        *hue = -1;
        return;
    }

    if (delta == 0)
    {
        *hue = 0;
        return;
    }

    if (rf == max)
    {
        *hue = (gf - bf) / delta; // between yellow & magenta
    }
    else if (gf == max)
    {
        *hue = 2 + (bf - rf) / delta; // between cyan & yellow
    }
    else
    {
        *hue = 4 + (rf - gf) / delta; // between magenta & cyan
    }

    *hue *= 60; // degrees

    if (*hue < 0)
    {
        *hue += 360;
    }
}

/**
 * @brief  灯带指示逻辑处理(仅库位变化时执行，灭灯的逻辑在business_type.c)
 * @param  pvParameters
 */
void ledStripIndicationTask(void *pvParameters)
{
    UBaseType_t _queueLen = 0;
    uint8_t _btightness = g_nvsData.DeviceConfigData.ledstripConfigData.btightness;
    uint16_t _orderOwnLedMaxNum = g_nvsData.projectConfigData.ledStripIndicationConfigData.orderOwnLedMaxNum;
    uint8_t _indicationModle = g_nvsData.projectConfigData.ledStripIndicationConfigData.indicationModle;
    for (;;)
    {
        xSemaphoreTake(g_ledStripBoxDataSemphHandle, portMAX_DELAY);
        _queueLen = uxQueueMessagesWaiting(g_ledStripBoxDataQueueHandler);
        BoxData_t _boxData = {0};
        ESP_LOGI(TAG, "--------start processing-------");
        uint8_t orderCount = 0;                                       // 库位实际订单数量统计
        uint8_t orderLocation[LED_STRIP_INDICATION_MAX_ORDERS] = {0}; // 订单所在数组位置记录
        for (size_t i = 0; i < _queueLen; i++)
        {
            if (xQueueReceive(g_ledStripBoxDataQueueHandler, &_boxData, 0) == pdPASS)
            {
                if (_boxData.isBoxOrderChanged)
                {
                    orderCount = 0;
                    for (size_t k = 0; k < LED_STRIP_INDICATION_MAX_ORDERS; k++)
                    {
                        if (_boxData.orderBoxInfo[k].orderNo != 0)
                        {
                            orderLocation[orderCount] = k; // 记录订单所在数组位置
                            orderCount++;
                        }
                    }
                    uint8_t ownerLedNum = (double)(_boxData.endLedId - _boxData.startLedId + 1) / (double)orderCount; // 单个订单能占用的最多灯珠数
                    // double _fraction = fmod(_num, 1.0);
                    // // 定义一个很小的阈值，用于比较double类型
                    // const double epsilon = 1e-9;
                    // if (fabs(_fraction - 0.5) < epsilon)
                    // {
                    //     ownerLedNum = (int)_num;
                    // }
                    // else if (_fraction > 0.5 || _fraction < -0.5)
                    // {
                    //     ownerLedNum = (int)(_num + 0.5);
                    // }
                    // else
                    // {
                    //     ownerLedNum = (int)_num;
                    // }
                    switch (_indicationModle)
                    {
                    case FIRST_COME_FIRST_SERVED_UNLIMITED_LEDS_MODE:
                        for (size_t j = 0; j < orderCount; j++) // 按照实际订单数赋予拥有值,并设置亮灯数据
                        {
                            _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId = _boxData.startLedId + (j * ownerLedNum);
                            _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.startLedId + ((j + 1) * ownerLedNum) - 1;
                            if (j == (orderCount - 1)) // 最后一个订单占满剩余灯珠，但是不超平均值
                            {
                                if (_boxData.endLedId - _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + 1 <= ownerLedNum)
                                {
                                    _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.endLedId;
                                }
                                else
                                {
                                    _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + ownerLedNum - 1;
                                }
                            }
                            uint8_t _red = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 16) & 0XFF);
                            uint8_t _green = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 8) & 0XFF);
                            uint8_t _blue = (_boxData.orderBoxInfo[orderLocation[j]].color & 0XFF);
                            float hue;          // hue (0 - 360)
                            uint8_t saturation; // saturation (0 - 255)
                            uint8_t value;      // value (0 - 255)
                            RGB8882HSV(_red, _green, _blue, &hue, &saturation, &value);
                            value = _btightness;
                            for (size_t x = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId; x <= _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId; x++)
                            {
                                led_strip_set_pixel_hsv(g_ledstripRmtHandle, x - 1, hue, saturation, value);
                            }
                        }
                        _boxData.isBoxOrderChanged = false; // 处理完成置位
                        xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_boxData, 0);
                        break;
                    case ORDER_MAXIMUM_LEDS_LIMIT_MODE:
                        if (ownerLedNum >= _orderOwnLedMaxNum && _orderOwnLedMaxNum != 0) // 订单实际能拥有灯珠限等于或超过允许分配数量
                        {
                            ownerLedNum = _orderOwnLedMaxNum;
                            for (size_t j = 0; j < orderCount; j++) // 按照实际订单数赋予拥有值,并设置亮灯数据
                            {
                                _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId = _boxData.startLedId + (j * ownerLedNum);
                                _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.startLedId + ((j + 1) * ownerLedNum) - 1;
                                if (j == (orderCount - 1)) // 最后一个订单占满剩余灯珠
                                {
                                    _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.endLedId;
                                    if (_boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId - _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + 1 > _orderOwnLedMaxNum) // 占满后超过最大订单灯珠限制，重新限定值
                                    {
                                        _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + _orderOwnLedMaxNum - 1;
                                    }
                                }
                                uint8_t _red = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 16) & 0XFF);
                                uint8_t _green = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 8) & 0XFF);
                                uint8_t _blue = (_boxData.orderBoxInfo[orderLocation[j]].color & 0XFF);
                                float hue;          // hue (0 - 360)
                                uint8_t saturation; // saturation (0 - 255)
                                uint8_t value;      // value (0 - 255)
                                RGB8882HSV(_red, _green, _blue, &hue, &saturation, &value);
                                value = _btightness;
                                for (size_t x = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId; x <= _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId; x++)
                                {
                                    led_strip_set_pixel_hsv(g_ledstripRmtHandle, x - 1, hue, saturation, value);
                                }
                            }
                        }
                        else if (ownerLedNum < _orderOwnLedMaxNum) // 订单实际能拥有灯珠限达不到最大允许分配数量，相当于均衡模式
                        {
                            for (size_t j = 0; j < orderCount; j++) // 按照实际订单数赋予拥有值,并设置亮灯数据
                            {
                                _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId = _boxData.startLedId + (j * ownerLedNum);
                                _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.startLedId + ((j + 1) * ownerLedNum) - 1;
                                if (j == (orderCount - 1)) // 最后一个订单占满剩余灯珠，但是不超平均值
                                {
                                    if (_boxData.endLedId - _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + 1 <= ownerLedNum)
                                    {
                                        _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.endLedId;
                                    }
                                    else
                                    {
                                        _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId + ownerLedNum - 1;
                                    }
                                }
                                uint8_t _red = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 16) & 0XFF);
                                uint8_t _green = ((_boxData.orderBoxInfo[orderLocation[j]].color >> 8) & 0XFF);
                                uint8_t _blue = (_boxData.orderBoxInfo[orderLocation[j]].color & 0XFF);
                                float hue;          // hue (0 - 360)
                                uint8_t saturation; // saturation (0 - 255)
                                uint8_t value;      // value (0 - 255)
                                RGB8882HSV(_red, _green, _blue, &hue, &saturation, &value);
                                value = _btightness;
                                for (size_t x = _boxData.orderBoxInfo[orderLocation[j]].ownerStartLedId; x <= _boxData.orderBoxInfo[orderLocation[j]].ownerEndLedId; x++)
                                {
                                    led_strip_set_pixel_hsv(g_ledstripRmtHandle, x - 1, hue, saturation, value);
                                }
                            }
                        }

                        _boxData.isBoxOrderChanged = false; // 处理完成置位
                        xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_boxData, 0);
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    // 库位没变化,不处理
                    xQueueSendToBack(g_ledStripBoxDataQueueHandler, &_boxData, 0);
                }
            }
        }
        led_strip_refresh(g_ledstripRmtHandle);
        ESP_LOGI(TAG, "--------End processing-------");
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}