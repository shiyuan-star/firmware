/**
 * @file ethernet.h
 * @brief 以太网头文件
 * @version 1.0
 * @date 2024-01-08
 * 
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _ETHERNET_H_
#define _ETHERNET_H_

#include "common.h"

#define ETH_SPI_HOST            1
#define ETH_SPI_CLOCK_MHZ       60
#define ETH_NETIF_KEY           "ETH_SPI1"
#define ETH_NETIF_DESC          "eth1"
// Ethernet SPI
#define ETH_SPI_PHY_ADDR0       1
#define ETH_SPI_PHY_RST0_GPIO   CONFIG_ETH_RST_PIN
#define ETH_SPI_INT0_GPIO       CONFIG_ETH_INT_PIN
#define ETH_SPI_CS0_GPIO        CONFIG_ETH_SCS_PIN
#define ETH_SPI_SCLK_GPIO       CONFIG_ETH_SCLK_PIN
#define ETH_SPI_MISO_GPIO       CONFIG_ETH_MISO_PIN
#define ETH_SPI_MOSI_GPIO       CONFIG_ETH_MOSI_PIN

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config)             \
    do                                                            \
    {                                                             \
        eth_module_config.spi_cs_gpio = ETH_SPI_CS0_GPIO;         \
        eth_module_config.int_gpio = ETH_SPI_INT0_GPIO;           \
        eth_module_config.phy_reset_gpio = ETH_SPI_PHY_RST0_GPIO; \
        eth_module_config.phy_addr = ETH_SPI_PHY_ADDR0;           \
    } while (0)

typedef enum
{
    ETH_MGR_INIT = 0x00,
    ETH_MGR_NETIF,
    ETH_MGR_EVENT_LOOP,
    ETH_MGR_SPI_NETIF,
    ETH_MGR_CONFIG,
    ETH_MGR_SPI_ISR,
    ETH_MGR_INIT_BUS,
    ETH_MGR_INIT_MODULE,
    ETH_MGR_INIT_MAC,
    ETH_MGR_INIT_PHY,
    ETH_MGR_SETUP_DRIVER,
    ETH_MGR_IOCTL,
    ETH_MGR_ATTACH,
    ETH_MGR_EVENT_HANDLER,
    ETH_MGR_START,
    ETH_MGR_CHECK,
    ETH_MGR_READY,
    ETH_MGR_QUIT
} EthernetManager_t;

typedef struct
{
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
} spi_eth_module_config_t;

extern EthernetManager_t getEthernetManager();
extern void ethConnMgrInit(EthConfigData_t *ethConfigData);
extern void switchEthConnMgr(EthernetManager_t ethMgr);
extern void ethConnManagerLoop();

#endif // _ETHERNET_H_
