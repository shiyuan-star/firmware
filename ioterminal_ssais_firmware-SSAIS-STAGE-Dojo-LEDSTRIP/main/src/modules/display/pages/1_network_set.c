/**
 * @file network_set.c
 * @brief 网络设置页面处理
 * @version 1.0
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */

#include "screen.h"

/**
 * @brief  网络设置页面处理
 * @param  controlId
 * @param  param
 * @return esp_err_t
 */
esp_err_t networkSetHandle(uint16_t controlId, uint8_t param[256], uint16_t size)
{
    static bool initialized = false;
    static uint8_t networkSelect = 0; // 网络连接选择 0:未设置  1：使用WiFi  2：使用以太网
    static EthConfigData_t ethConfigData;
    static WifiConfigData_t wifiConfigData;
    if (!initialized)
    {
        ethConfigData = g_nvsData.networkConfigData.ethConfigData;
        wifiConfigData = g_nvsData.networkConfigData.wifiConfigData;
        initialized = true;
    }

    switch (controlId)
    {
    case SCREEN_ETH_NETWORK_ENABLE_BUTTON:
        if (param[1] == true)
        {
            networkSelect = 2;
        }
        else
        {
            networkSelect = 1;
        }
        break;
    case SCREEN_ETH_STATIC_IP_TEXT:
        strcpy(ethConfigData.staticIp, (char *)param);
        break;
    case SCREEN_ETH_NET_MASK_TEXT:
        strcpy(ethConfigData.netMask, (char *)param);
        break;
    case SCREEN_ETH_GATEWAY_TEXT:
        strcpy(ethConfigData.gateway, (char *)param);
        break;
    case SCREEN_ETH_DHCP_ENABLE_BUTTON:
        ethConfigData.ethDhcpEnabled = param[1];
        break;
    case SCREEN_WIFI_NETWORK_ENABLE_BUTTON:
        if (param[1] == true)
        {
            networkSelect = 1;
        }
        else
        {
            networkSelect = 2;
        }
        break;
    case SCREEN_WIFI_SSID_TEXT:
        strcpy(wifiConfigData.ssid, (char *)param);
        break;
    case SCREEN_WIFI_PASS_TEXT:
        strcpy(wifiConfigData.password, (char *)param);
        break;

    case SCREEN_NETWORK_SAVE_BUTTON:
        if (!param[1]) // 按键松开不处理，按下时保存
        {
            break;
        }
        if (networkSelect != 0) // 修改了网络连接方式
        {
            if (networkSelect == 1 && g_nvsData.networkConfigData.wifiConfigData.wifiEnabled == false)
            {
                wifiConfigData.wifiEnabled = true;
                ethConfigData.ethernetEnabled = false;
            }
            if (networkSelect == 2 && g_nvsData.networkConfigData.ethConfigData.ethernetEnabled == false)
            {
                wifiConfigData.wifiEnabled = false;
                ethConfigData.ethernetEnabled = true;
            }
        }

        if (configStructCmp(&wifiConfigData, &(g_nvsData.networkConfigData.wifiConfigData), WifiConfigData_reflection) &&
            configStructCmp(&ethConfigData, &(g_nvsData.networkConfigData.ethConfigData), EthConfigData_reflection))
        {
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "配置未更新", "Configuration not changed", "構成が更新されていません");
        }
        else
        {
            g_nvsData.networkConfigData.wifiConfigData = wifiConfigData;
            g_nvsData.networkConfigData.ethConfigData = ethConfigData;
            saveConfigToNvs(&g_nvsData);
            setTextValueMultilingual(SCREEN_MESSAGE_DIALOG_PAGE, SCREEN_MESSAGE_DAILOG_INFO_TEXT, "保存成功", "Successfully saved", "保存に成功しました");
        }

        break;
    default:
        break;
    }
    return ESP_OK;
}
