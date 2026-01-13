/**
 * @file ssais_ledstrip_set.c
 * @brief ssais灯带设置
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

#define BEGIN_LED_TEXT_UPDATE SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_BEGIN_LED_TEXT, ledStripDebug.beginNum)
#define END_LED_TEXT_UPDATE SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_END_LED_TEXT, ledStripDebug.endNum)

static const char *TAG = "SSAIA_LEDSTRIP_SET";

typedef struct _LedStripDebug
{
    bool enable;
    bool autoSync;
    uint16_t beginNum;
    uint16_t endNum;
    uint8_t brightness;
    uint16_t initLednum; // 初始化led灯带时，设定的led数量
} LedStripDebug_t;

// const cjsonx_reflect_t BoxParam_reflection[] = {
//     __cjsonx_int(BoxParam_t, maxX),
//     __cjsonx_int(BoxParam_t, minX),
//     __cjsonx_int(BoxParam_t, maxY),
//     __cjsonx_int(BoxParam_t, minY),
//     __cjsonx_int(BoxParam_t, beginLed),
//     __cjsonx_int(BoxParam_t, endLed),
//     __cjsonx_str(BoxParam_t, boxName),
//     __cjsonx_end()};

/**
 * @brief  设置灯带首尾亮灯
 * @param  beginLed 起始灯珠为1
 * @param  endLed
 * @param  brightness
 */
static void setLedstripBeginEnd(uint16_t beginLed, uint16_t endLed, uint8_t brightness)
{
    LEDSTRIP_CLEAR;
    for (size_t i = beginLed; i <= endLed; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, i - 1, brightness, brightness, brightness);
    }
}

/**
 * @brief  ssais灯带设置
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t ssaisProjectHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static LedStripDebug_t ledStripDebug;
    static bool initialized = false;
    if (!initialized)
    {
        ledStripDebug.enable = g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled; // 灯带的使能状态
        ledStripDebug.autoSync = true;
        ledStripDebug.beginNum = SCREEN_SSAIS_PROJECT_BEGIN_LED_TEXT_DEFAULT_DATA;
        ledStripDebug.endNum = SCREEN_SSAIS_PROJECT_END_LED_TEXT_DEFAULT_DATA;
        ledStripDebug.brightness = g_nvsData.DeviceConfigData.ledstripConfigData.btightness;
        ledStripDebug.initLednum = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
        initialized = true;
    }
    switch (controlId)
    {

    case SCREEN_SSAIS_PROJECT_RETURN_BUTTON:
        SetButtonValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_ALLOW_CAPTURE_IRTOUCH_DATA_BUTTON, false);
        xEventGroupSetBits(g_ifTouchDataFLowEventGroup, TOUCH_CENTER_MODE_BIT);
        xEventGroupClearBits(g_ifTouchDataFLowEventGroup, TOUCH_MIN_MAX_MODE_BIT);
        break;

    case SCREEN_ALLOW_CAPTURE_IRTOUCH_DATA_BUTTON:
        bool captureInfraredTouchData = param[1];
        if (captureInfraredTouchData) // 截取红外触摸数据
        {
            xEventGroupSetBits(g_ifTouchDataFLowEventGroup, TOUCH_MIN_MAX_MODE_BIT);
            xEventGroupClearBits(g_ifTouchDataFLowEventGroup, TOUCH_CENTER_MODE_BIT);
        }
        else
        {
            xEventGroupSetBits(g_ifTouchDataFLowEventGroup, TOUCH_CENTER_MODE_BIT);
            xEventGroupClearBits(g_ifTouchDataFLowEventGroup, TOUCH_MIN_MAX_MODE_BIT);
        }
        break;

    case SCREEN_SSAIS_PROJECT_REDRAW_BUTTON:
        g_drawBoxParam.maxX = 0;
        g_drawBoxParam.minX = INFRARED_TOUCH_DATA_VALUE_MAXNUM;
        g_drawBoxParam.maxY = 0;
        g_drawBoxParam.minY = INFRARED_TOUCH_DATA_VALUE_MAXNUM;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_X_TEXT, INFRARED_TOUCH_DATA_VALUE_MAXNUM);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_X_TEXT, 0);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_Y_TEXT, INFRARED_TOUCH_DATA_VALUE_MAXNUM);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_Y_TEXT, 0);
        break;

    case SCREEN_SSAIS_PROJECT_BEGIN_LED_TEXT:
        ledStripDebug.beginNum = str2num((char *)param);
        if (ledStripDebug.beginNum >= ledStripDebug.initLednum)
        {
            ledStripDebug.beginNum = ledStripDebug.initLednum;
            BEGIN_LED_TEXT_UPDATE;
        }
        if (ledStripDebug.endNum < ledStripDebug.beginNum)
        {
            ledStripDebug.endNum = ledStripDebug.beginNum;
            END_LED_TEXT_UPDATE;
        }
        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_BEGIN_LED_UP_BUTTON:
        if (ledStripDebug.beginNum >= ledStripDebug.initLednum)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        ledStripDebug.beginNum++;
        if (ledStripDebug.endNum < ledStripDebug.beginNum)
        {
            ledStripDebug.endNum = ledStripDebug.beginNum;
            END_LED_TEXT_UPDATE;
        }
        BEGIN_LED_TEXT_UPDATE;
        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_BEGIN_LED_DOWN_BUTTON:
        if (ledStripDebug.beginNum <= 1)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        ledStripDebug.beginNum--;
        BEGIN_LED_TEXT_UPDATE;
        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_END_LED_TEXT:
        ledStripDebug.endNum = str2num((char *)param);
        if (ledStripDebug.endNum >= ledStripDebug.initLednum)
        {
            ledStripDebug.endNum = ledStripDebug.initLednum;
            END_LED_TEXT_UPDATE;
        }
        if (ledStripDebug.endNum < ledStripDebug.beginNum)
        {
            ledStripDebug.beginNum = ledStripDebug.endNum;
            BEGIN_LED_TEXT_UPDATE;
        }
        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_END_LED_UP_BUTTON:
        if (ledStripDebug.endNum >= ledStripDebug.initLednum)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        ledStripDebug.endNum++;
        END_LED_TEXT_UPDATE;

        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_END_LED_DOWN_BUTTON:
        if (ledStripDebug.endNum <= 1)
        {
            return ESP_ERR_INVALID_SIZE;
        }
        ledStripDebug.endNum--;
        if (ledStripDebug.endNum < ledStripDebug.beginNum)
        {
            ledStripDebug.beginNum = ledStripDebug.endNum;
            BEGIN_LED_TEXT_UPDATE;
        }
        END_LED_TEXT_UPDATE;
        setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
        if (ledStripDebug.autoSync)
        {
            LEDSTRIP_REFRESH;
        }
        break;

    case SCREEN_SSAIS_PROJECT_BOX_NAME_TEXT:
        strcpy(g_drawBoxParam.boxName, (char *)param);
        break;

    case SCREEN_SSAIS_PROJECT_SAVE_BOX_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        g_drawBoxParam.beginLed = ledStripDebug.beginNum;
        g_drawBoxParam.endLed = ledStripDebug.endNum;
        ESP_ERROR_CHECK(screenModifyBoxInfoSaveToNvs()); // 写入单个库位
        g_drawBoxParam.maxX = 0;
        g_drawBoxParam.minX = INFRARED_TOUCH_DATA_VALUE_MAXNUM;
        g_drawBoxParam.maxY = 0;
        g_drawBoxParam.minY = INFRARED_TOUCH_DATA_VALUE_MAXNUM;
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_X_TEXT, INFRARED_TOUCH_DATA_VALUE_MAXNUM);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_X_TEXT, 0);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MAX_Y_TEXT, INFRARED_TOUCH_DATA_VALUE_MAXNUM);
        SetTextNumValue(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_MIN_Y_TEXT, 0);
        SetTextClena(SCREEN_SSAIS_PROJECT_SET_PAGE, SCREEN_SSAIS_PROJECT_BOX_NAME_TEXT);
        break;

    case SCREEN_SSAIS_PROJECT_DELETE_ALL_BOX_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时删除
        {
            break;
        }
        g_screenState.waitCheckEvent = DELETE_ALL_BOX_CHECK;
        setTextValueMultilingual(SCREEN_CHECK_DIALOG_PAGE, SCREEN_CHECK_DAILOG_INFO_TEXT, "确认删除所有已存储的库位配置?", "Are you sure to delete all stored storage location configurations?", "すべてのリポジトリビット構成の削除を確認しますか？");
        break;

    case SCREEN_SSAIS_PROJECT_DEBUG_BUTTON:
        char *boxParamStr = readBoxParamFromNvs();
        if (boxParamStr == NULL)
        {
            ESP_LOGE(TAG, "Failed to read box param from NVS.");
        }
        else
        {
            ESP_LOGI(TAG, "BoxParam: %s", boxParamStr);
            heap_caps_free(boxParamStr);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}
