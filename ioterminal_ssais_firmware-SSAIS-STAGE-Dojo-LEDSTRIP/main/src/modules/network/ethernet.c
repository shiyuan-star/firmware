/**
 * @file ethernet.c
 * @brief ETH 驱动与状态机
 * @version 1.0
 * @date 2024-01-18
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "ethernet.h"

static const char *TAG = "ETHERNET";
static EthernetManager_t s_ethernetManager = ETH_MGR_INIT;
static EthernetManager_t s_lastEthernetManager = ETH_MGR_INIT;
static esp_netif_t *p_eth_netif_spi = NULL;
static esp_eth_mac_t *p_mac_spi = NULL;
static esp_eth_phy_t *p_phy_spi = NULL;
static uint8_t s_ethMac[16] = {0};
static esp_eth_handle_t s_eth_handle;
static spi_eth_module_config_t s_spi_eth_module_config;
static EthConfigData_t s_ethConfigData;

/**
 * @brief  Event handler for Ethernet events
 * @param  arg
 * @param  event_base
 * @param  event_id
 * @param  event_data
 */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Link Up");
        ESP_LOGI(TAG, "HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        NETWORT_STATE_UPDATE(NET_DISCONNECT);
        ESP_LOGI(TAG, "Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Stopped");
        break;
    default:
        break;
    }
}

/**
 * @brief  Event handler for IP_EVENT_ETH_GOT_IP
 * @param  arg
 * @param  event_base
 * @param  event_id
 * @param  event_data
 */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Got IP: %d.%d.%d.%d, MASK: %d.%d.%d.%d, GW: %d.%d.%d.%d", IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask), IP2STR(&ip_info->gw));
    if (xSemaphoreTake(g_sysStateInfoMetex, portMAX_DELAY) == pdTRUE)
    {
        sprintf(g_sysStateInfo.ipAddr, IPSTR, IP2STR(&ip_info->ip));
        g_sysStateInfo.network = ETH_CONNECT;
        xSemaphoreGive(g_sysStateInfoMetex);
    }
}

/**
 * @brief 获取ETH连接状态机状态
 * @return EthernetManager_t
 */
EthernetManager_t getEthernetManager()
{
    return s_ethernetManager;
}

/**
 * @brief ETH连接状态机初始化
 * @param  ethConfigData
 */
void ethConnMgrInit(EthConfigData_t *ethConfigData)
{
    s_ethConfigData.ethernetEnabled = ethConfigData->ethernetEnabled;
    s_ethConfigData.ethDhcpEnabled = ethConfigData->ethDhcpEnabled;
    strcpy(s_ethConfigData.staticIp, ethConfigData->staticIp);
    strcpy(s_ethConfigData.netMask, ethConfigData->netMask);
    strcpy(s_ethConfigData.gateway, ethConfigData->gateway);
    if (getEthernetManager() != ETH_MGR_INIT)
    {
        switchEthConnMgr(ETH_MGR_QUIT);
        while (s_lastEthernetManager != s_ethernetManager)
        {
            ethConnManagerLoop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    switchEthConnMgr(ETH_MGR_INIT);
}

/**
 * @brief  切换ETH状态机事件组
 * @param  ethMgr
 */
void switchEthConnMgr(EthernetManager_t ethMgr)
{
    if (ethMgr != s_ethernetManager)
    {
        s_lastEthernetManager = s_ethernetManager;
        s_ethernetManager = ethMgr;
        // ESP_LOGI(TAG, "EthernetManager %d -> %d", s_lastEthernetManager, s_ethernetManager);
    }
}

/**
 * @brief ETH连接状态机循环
 */
void ethConnManagerLoop()
{
    switch (s_ethernetManager)
    {
    case ETH_MGR_INIT:
    {
        ESP_LOGW(TAG, "Ethernet started");
        g_sysStateInfo.network = NET_DISCONNECT;
        esp_err_t err = esp_efuse_mac_get_default(s_ethMac);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to get MAC from EFUSE.");
            break;
        }
        switchEthConnMgr(ETH_MGR_NETIF);
        break;
    }

    case ETH_MGR_NETIF:
    {
        // Initialize TCP/IP network interface (should be called only once in application)
        ESP_ERROR_CHECK(esp_netif_init());
        switchEthConnMgr(ETH_MGR_EVENT_LOOP);
        break;
    }

    case ETH_MGR_EVENT_LOOP:
    {
        // Create default event loop that running in background
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        switchEthConnMgr(ETH_MGR_SPI_NETIF);
        break;
    }

    case ETH_MGR_SPI_NETIF:
    {
        // Create instance(s) of esp-netif for SPI Ethernet(s)
        esp_netif_inherent_config_t netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
        netif_config.if_key = ETH_NETIF_KEY;
        netif_config.if_desc = ETH_NETIF_DESC;
        netif_config.route_prio = 30 - 1;
        esp_netif_config_t cfg_spi = {
            .base = &netif_config,
            .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
        p_eth_netif_spi = esp_netif_new(&cfg_spi);
        switchEthConnMgr(ETH_MGR_CONFIG);
        break;
    }

    case ETH_MGR_CONFIG:
    {
        if (s_ethConfigData.ethDhcpEnabled) // 使用DHCP配置网络
        {
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcpc_start(p_eth_netif_spi));
        }
        else
        {
            ESP_LOGI(TAG, "ETH set static IP");
            esp_netif_ip_info_t ipUserSet;
            memset(&ipUserSet, 0, sizeof(esp_netif_ip_info_t));
            ipUserSet.ip.addr = ipaddr_addr(s_ethConfigData.staticIp);
            ipUserSet.gw.addr = ipaddr_addr(s_ethConfigData.gateway);
            ipUserSet.netmask.addr = ipaddr_addr(s_ethConfigData.netMask);
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcpc_stop(p_eth_netif_spi));
            ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_set_ip_info(p_eth_netif_spi, &ipUserSet));
        }
        switchEthConnMgr(ETH_MGR_SPI_ISR);
        break;
    }

    case ETH_MGR_SPI_ISR:
    {
        // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
        gpio_install_isr_service(0);
        switchEthConnMgr(ETH_MGR_INIT_BUS);
        break;
    }

    case ETH_MGR_INIT_BUS:
    {
        // Init SPI bus
        spi_bus_config_t buscfg = {
            .miso_io_num = ETH_SPI_MISO_GPIO,
            .mosi_io_num = ETH_SPI_MOSI_GPIO,
            .sclk_io_num = ETH_SPI_SCLK_GPIO,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
        };
        ESP_ERROR_CHECK(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
        switchEthConnMgr(ETH_MGR_INIT_MODULE);
        break;
    }

    case ETH_MGR_INIT_MODULE:
    {
        // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
        INIT_SPI_ETH_MODULE_CONFIG(s_spi_eth_module_config);
        switchEthConnMgr(ETH_MGR_INIT_MAC);
        break;
    }

    case ETH_MGR_INIT_MAC:
    {
        // Configure SPI interface for specific SPI module
        spi_device_interface_config_t spi_devcfg = {
            .mode = 0,
            .clock_speed_hz = ETH_SPI_CLOCK_MHZ * 1000 * 1000,
            .queue_size = 20,
            .spics_io_num = s_spi_eth_module_config.spi_cs_gpio};
        // Init vendor specific MAC config to default
        eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
        eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &spi_devcfg);
        w5500_config.int_gpio_num = s_spi_eth_module_config.int_gpio;
        // Create new SPI Ethernet MAC instance
        p_mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
        switchEthConnMgr(ETH_MGR_INIT_PHY);
        break;
    }

    case ETH_MGR_INIT_PHY:
    {
        // Init PHY configs to default
        eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
        // Set remaining GPIO numbers and configuration used by the SPI module
        phy_config.phy_addr = s_spi_eth_module_config.phy_addr;
        phy_config.reset_gpio_num = s_spi_eth_module_config.phy_reset_gpio;
        p_phy_spi = esp_eth_phy_new_w5500(&phy_config);
        switchEthConnMgr(ETH_MGR_SETUP_DRIVER);
        break;
    }

    case ETH_MGR_SETUP_DRIVER:
    {
        esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(p_mac_spi, p_phy_spi);
        esp_err_t err = esp_eth_driver_install(&eth_config_spi, &s_eth_handle);
        if (err != ESP_OK)
        {
            break;
        }
        switchEthConnMgr(ETH_MGR_IOCTL);
        break;
    }

    case ETH_MGR_IOCTL:
    {
        // The SPI Ethernet module might not have a burned factory MAC address, we cat to set it manually.
        ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, s_ethMac));
        switchEthConnMgr(ETH_MGR_ATTACH);
        break;
    }

    case ETH_MGR_ATTACH:
    {
        // attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(p_eth_netif_spi, esp_eth_new_netif_glue(s_eth_handle)));
        switchEthConnMgr(ETH_MGR_EVENT_HANDLER);
        break;
    }

    case ETH_MGR_EVENT_HANDLER:
    {
        // Register user defined event handers
        ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
        ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));
        switchEthConnMgr(ETH_MGR_START);
        break;
    }

    case ETH_MGR_START:
    {
        ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));
        switchEthConnMgr(ETH_MGR_CHECK);
        break;
    }

    case ETH_MGR_CHECK:
    {
        if (g_sysStateInfo.network == ETH_CONNECT)
        {
            mqttDefaultTopicPubStrMsg(MQTT_CONTROL_TYPE_NETWORK_STATE, NOTIFY_NETWORK_IP_ADDR, "ip", g_sysStateInfo.ipAddr);
            ESP_LOGW(TAG, "Ethernet ready.");
            switchEthConnMgr(ETH_MGR_READY);
        }
        break;
    }

    case ETH_MGR_READY:
    {
        break;
    }

    case ETH_MGR_QUIT:
    {
        if (s_lastEthernetManager != s_ethernetManager)
        {
            g_sysStateInfo.network = NET_DISCONNECT;
            if (s_lastEthernetManager >= ETH_MGR_START)
            {
                ESP_ERROR_CHECK(esp_eth_stop(s_eth_handle));
            }
            if (s_lastEthernetManager >= ETH_MGR_EVENT_HANDLER)
            {
                ESP_ERROR_CHECK(esp_event_handler_instance_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler));
                esp_eth_decrease_reference(s_eth_handle);
                ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
                esp_eth_decrease_reference(s_eth_handle);
            }
            if (s_lastEthernetManager >= ETH_MGR_SETUP_DRIVER)
            {
                esp_eth_driver_uninstall(s_eth_handle);
            }
            if (s_lastEthernetManager >= ETH_MGR_INIT_MAC)
            {
                p_mac_spi->del(p_mac_spi);
            }
            if (s_lastEthernetManager >= ETH_MGR_INIT_PHY)
            {
                p_phy_spi->del(p_phy_spi);
            }
            if (s_lastEthernetManager >= ETH_MGR_INIT_BUS)
            {
                spi_bus_free(ETH_SPI_HOST);
            }
            if (s_lastEthernetManager >= ETH_MGR_SPI_ISR)
            {
                gpio_uninstall_isr_service();
            }
            if (s_lastEthernetManager >= ETH_MGR_SPI_NETIF)
            {
                esp_netif_destroy(p_eth_netif_spi);
            }
            if (s_lastEthernetManager >= ETH_MGR_EVENT_LOOP)
            {
                esp_event_loop_delete_default();
            }
            if (s_lastEthernetManager >= ETH_MGR_NETIF)
            {
                esp_netif_deinit();
            }
            s_lastEthernetManager = s_ethernetManager;
            ESP_LOGW(TAG, "Ethernet stopped");
        }
        break;
    }
    }
}
