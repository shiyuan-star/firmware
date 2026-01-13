/**
 * @file 10_dio_set.c
 * @brief dio设置页面
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "screen.h"

/**
 * @brief  dio设置页面
 * @param  controlId
 * @param  param
 * @param  size
 * @return esp_err_t
 */
esp_err_t dioSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{

    switch (controlId)
    {
    case SCREEN_DIO_SET_CMD_SEND_BUTTON:
        char chinese[256], english[256], japan[256];
        sprintf(chinese, "不支持外接RS485设备");
        sprintf(english, "External RS485 device not supported");
        sprintf(japan, "外部RS485デバイスはサポートされていません");
        setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, chinese, english, japan);
        break;
    default:
        break;
    }
    return ESP_OK;

    // static uint8_t nowSendAddr = 0;
    // static uint8_t writeAddr = 0;
    // switch (controlId)
    // {
    // case SCREEN_DIO_SET_ONELINE_ADDR_1_BUTTON:
    //     nowSendAddr = 1;
    //     break;
    // case SCREEN_DIO_SET_ONELINE_ADDR_2_BUTTON:
    //     nowSendAddr = 2;
    //     break;
    // case SCREEN_DIO_SET_ONELINE_ADDR_3_BUTTON:
    //     nowSendAddr = 3;
    //     break;
    // case SCREEN_DIO_SET_WRITE_ADDR_SELECT_TEXT:
    //     writeAddr = str2num((char *)param);
    //     break;
    // case SCREEN_DIO_SET_CMD_SEND_BUTTON:
    //     if (!param[1]) // 按键松开不处理，按下发送
    //     {
    //         break;
    //     }
    //     if (nowSendAddr == 0 || writeAddr == 0)
    //     {
    //         setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "设定参数无效", "Invalid parameter settings", "設定パラメータが無効です");
    //         break;
    //     }
    //     esp_err_t err;
    //     char chinese[256], english[256], japan[256];
    //     err = setDioAddr(nowSendAddr, writeAddr);
    //     if (err != ESP_OK)
    //     {
    //         sprintf(chinese, "设定通信地址[%d]的设备,新地址为[%d],设置失败,请检查设备连接", nowSendAddr, writeAddr);
    //         sprintf(english, "Set the communication address [%d] for the device, and the new address is [%d],Setting failed, please check device connection", nowSendAddr, writeAddr);
    //         sprintf(japan, "通信アドレス[%d]を設定するデバイス、新しいアドレス[%d],設定に失敗しました。デバイス接続を確認してください", nowSendAddr, writeAddr);
    //         setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, chinese, english, japan);
    //         break;
    //     }
    //     else
    //     {
    //         sprintf(chinese, "设定通信地址[%d]的设备,新地址为[%d],设置成功,重启DIO生效", nowSendAddr, writeAddr);
    //         sprintf(english, "Set the communication address [%d] for the device, and the new address is [%d],Setting successful, restart DIO to take effect", nowSendAddr, writeAddr);
    //         sprintf(japan, "通信アドレス[%d]を設定するデバイス、新しいアドレス[%d],設定に成功し、DIOを再起動して有効になります,", nowSendAddr, writeAddr);
    //         setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, chinese, english, japan);
    //         if (nowSendAddr == 1)
    //         {
    //             SetTextNumValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_1_TEXT, writeAddr);
    //         }
    //         else if (nowSendAddr == 2)
    //         {
    //             SetTextNumValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_2_TEXT, writeAddr);
    //         }
    //         else if (nowSendAddr == 3)
    //         {
    //             SetTextNumValue(SCREEN_DIO_SET_PAGE, SCREEN_DIO_SET_WRITE_ADDR_3_TEXT, writeAddr);
    //         }
    //         break;
    //     }
    //     break;
    // case SCREEN_DIO_SET_DIO1_ON_OFF_BUTTON:
    //     if (param[1]) // 按钮按下，打开输出
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR1, true);
    //     }
    //     else
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR1, false);
    //     }
    //     break;
    // case SCREEN_DIO_SET_DIO2_ON_OFF_BUTTON:
    //     if (param[1]) // 按钮按下，打开输出
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR2, true);
    //     }
    //     else
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR2, false);
    //     }
    //     break;
    // case SCREEN_DIO_SET_DIO3_ON_OFF_BUTTON:
    //     if (param[1]) // 按钮按下，打开输出
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR3, true);
    //     }
    //     else
    //     {
    //         setDioOnOff(MB_SLAVE_ADDR3, false);
    //     }
    //     break;

    // case SCREEN_DIO_SET_RETURN_BUTTON:
    //     nowSendAddr = 0;
    //     writeAddr = 0;
    //     switchMbManager(MODBUS_MGR_SET_DIO_FINISH);
    //     break;
    // default:
    //     break;
    // }
    // return ESP_OK;
}
