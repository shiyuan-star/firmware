/**
 * @file 8_ledstrip_debug.c
 * @brief 灯带调试页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

#define BEGIN_LED_TEXT_UPDATE SetTextNumValue(SCREEN_LEDSTRIP_DEBUG_PAGE, SCREEN_LEDSTRIP_BEGIN_LED_TEXT, ledStripDebug.beginNum)
#define END_LED_TEXT_UPDATE SetTextNumValue(SCREEN_LEDSTRIP_DEBUG_PAGE, SCREEN_LEDSTRIP_END_LED_TEXT, ledStripDebug.endNum)
#define SET_LED_INTERVAL_TEXT_UPDATE SetTextNumValue(SCREEN_LEDSTRIP_DEBUG_PAGE, SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_TEXT, ledStripDebug.setInterval)
#define SET_LED_INTERVAL_NUM_TEXT_UPDATE SetTextNumValue(SCREEN_LEDSTRIP_DEBUG_PAGE, SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_NUM_TEXT, ledStripDebug.setIntervalNum)

#define SET_LEDSTRIP_BEGIN_END_MODE 1
#define SET_LEDSTRIP_INTERVAL_MODE 2
typedef struct _LedStripDebug
{
    bool enable;
    bool autoSync;
    uint8_t debugmode; // 调试模式，起始-结束1   间隔调节：2
    uint16_t beginNum;
    uint16_t endNum;
    uint8_t brightness;
    uint16_t setInterval;
    uint16_t setIntervalNum;
    uint16_t initLednum; // 初始化led灯带时，设定的led数量
} LedStripDebug_t;

/**
 * @brief  设置灯带首尾亮灯
 * @param  beginLed 起始灯珠为1
 * @param  endLed
 * @param  brightness
 */
void setLedstripBeginEnd(uint16_t beginLed, uint16_t endLed, uint8_t brightness)
{
    led_strip_clear(g_ledstripRmtHandle);
    for (size_t i = beginLed; i <= endLed; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, i - 1, brightness, brightness, brightness);
    }
}

/**
 * @brief  设置灯带亮灯间隔
 * @param  setInterval
 * @param  setIntervalNum
 * @param  brightness
 */
void setLedInterval(uint16_t setInterval, uint16_t setIntervalNum, uint8_t brightness)
{
    led_strip_clear(g_ledstripRmtHandle);
    for (size_t i = 1; i * setInterval <= setIntervalNum; i++)
    {
        led_strip_set_pixel(g_ledstripRmtHandle, (i * setInterval) - 1, brightness, brightness, brightness);
    }
}

/**
 * @brief  灯带设置页面处理
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t ledstripDebugHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static LedStripDebug_t ledStripDebug;
    static bool initialized = false;
    if (!initialized)
    {
        ledStripDebug.enable = g_nvsData.DeviceConfigData.ledstripConfigData.ledstripEnabled; // 灯带的使能状态
        ledStripDebug.autoSync = true;                                                        // 默认开启同步
        ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
        ledStripDebug.beginNum = SCREEN_LEDSTRIP_BEGIN_LED_TEXT_DEFAULT_DATA;
        ledStripDebug.endNum = SCREEN_LEDSTRIP_END_LED_TEXT_DEFAULT_DATA;
        ledStripDebug.brightness = SCREEN_LEDSTRIP_DEBUG_BRIGHTNESS_TEXT_DEFAULT_DATA;
        ledStripDebug.setInterval = SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_DEFAULT_DATA;
        ledStripDebug.setIntervalNum = SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_NUM_DEFAULT_DATA;
        ledStripDebug.initLednum = g_nvsData.DeviceConfigData.ledstripConfigData.ledNum;
        initialized = true;
    }
    if (ledStripDebug.enable) // 初始化过灯带，允许全部调试功能
    {
        switch (controlId)
        {
        case SCREEN_LEDSTRIP_BEGIN_LED_TEXT:
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
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_BEGIN_LED_UP_BUTTON:
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
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_BEGIN_LED_DOWN_BUTTON:
            if (ledStripDebug.beginNum <= 1)
            {
                return ESP_ERR_INVALID_SIZE;
            }
            ledStripDebug.beginNum--;
            BEGIN_LED_TEXT_UPDATE;
            setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_CLEAR_BUTTON:
            led_strip_clear(g_ledstripRmtHandle);
            ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            break;

        case SCREEN_LEDSTRIP_AUTO_SYNC_BUTTON:
            ledStripDebug.autoSync = param[1];
            break;

        case SCREEN_LEDSTRIP_END_LED_TEXT:
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
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_END_LED_UP_BUTTON:
            if (ledStripDebug.endNum >= ledStripDebug.initLednum)
            {
                return ESP_ERR_INVALID_SIZE;
            }
            ledStripDebug.endNum++;
            END_LED_TEXT_UPDATE;

            setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_END_LED_DOWN_BUTTON:
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
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_BEGIN_END_MODE;
            break;

        case SCREEN_LEDSTRIP_BRIGHTNESS_PROGRESS:
            ledStripDebug.brightness = PTR2U32(param);
            if (ledStripDebug.debugmode == SET_LEDSTRIP_BEGIN_END_MODE)
            {
                setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
            }
            else if (ledStripDebug.debugmode == SET_LEDSTRIP_INTERVAL_MODE)
            {
                setLedInterval(ledStripDebug.setInterval, ledStripDebug.setIntervalNum, ledStripDebug.brightness);
            }
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            break;

        case SCREEN_LEDSTRIP_DEBUG_BRIGHTNESS_TEXT:
            ledStripDebug.brightness = str2num((char *)param);
            if (ledStripDebug.debugmode == SET_LEDSTRIP_BEGIN_END_MODE)
            {
                setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
            }
            else if (ledStripDebug.debugmode == SET_LEDSTRIP_INTERVAL_MODE)
            {
                setLedInterval(ledStripDebug.setInterval, ledStripDebug.setIntervalNum, ledStripDebug.brightness);
            }
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            break;

        case SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_TEXT:
            ledStripDebug.setInterval = str2num((char *)param);
            if (ledStripDebug.setInterval > ledStripDebug.initLednum)
            {
                ledStripDebug.setInterval = ledStripDebug.initLednum;
                SET_LED_INTERVAL_TEXT_UPDATE;
            }
            setLedInterval(ledStripDebug.setInterval, ledStripDebug.setIntervalNum, ledStripDebug.brightness);
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_INTERVAL_MODE;
            break;

        case SCREEN_LEDSTRIP_DEBUG_LED_INTERVAL_NUM_TEXT:
            ledStripDebug.setIntervalNum = str2num((char *)param);
            if (ledStripDebug.setIntervalNum > ledStripDebug.initLednum)
            {
                ledStripDebug.setIntervalNum = ledStripDebug.initLednum;
                SET_LED_INTERVAL_NUM_TEXT_UPDATE;
            }
            setLedInterval(ledStripDebug.setInterval, ledStripDebug.setIntervalNum, ledStripDebug.brightness);
            if (ledStripDebug.autoSync)
            {
                ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            }
            ledStripDebug.debugmode = SET_LEDSTRIP_INTERVAL_MODE;
            break;

        case SCREEN_LEDSTRIP_DEBUG_LED_SYNC_BUTTON:
            if (ledStripDebug.debugmode == SET_LEDSTRIP_BEGIN_END_MODE)
            {
                setLedstripBeginEnd(ledStripDebug.beginNum, ledStripDebug.endNum, ledStripDebug.brightness);
            }
            else if (ledStripDebug.debugmode == SET_LEDSTRIP_INTERVAL_MODE)
            {
                setLedInterval(ledStripDebug.setInterval, ledStripDebug.setIntervalNum, ledStripDebug.brightness);
            }
            ESP_ERROR_CHECK(led_strip_refresh(g_ledstripRmtHandle));
            break;

        default:
            break;
        }
        return ESP_OK;
    }
    else // 未初始化过灯带 直接返回
    {
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "灯带未初始化，请检查是否启用。", "The light strip is not initialized, please check if it is enabled.", "ランプバンドは初期化されていません。有効かどうかを確認してください");
        setScreenPage(SCREEN_MESSAGE_DIALOG_PAGE);
        return ESP_OK;
    }
}
