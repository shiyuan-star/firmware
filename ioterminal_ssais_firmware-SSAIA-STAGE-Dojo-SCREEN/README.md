[TOC]

**IoTerminal_SSAIS_Firmware**

| Supported Hardware | IoTerminal_S3_V4.x |
| ------------------ | ------------------ |

| Supported ESP_IDF_VER | V5.2.1 |
| --------------------- | ------ |

# 硬件
## 硬件V4.0.1接口引脚

### 板载LED

|    功能     | 引脚号 |      备注      |
| :-------: | :-: | :----------: |
| State_LED | 42  | 板载绿色指示灯低电平亮灯 |

### 以太网W5500
|    功能    | 引脚号 |
| :------: | :-: |
| ETH_SCLK | 36  |
| ETH_MISO | 47  |
| ETH_MOSI | 21  |
| ETH_SCS  | 48  |
| ETH_INT  | 14  |
| ETH_RST  | 13  |

### LED灯带
|     功能      | 引脚号 |
| :---------: | :-: |
| LedStrip_IO | 10  |


### 串口屏

|      功能      | 引脚号 |        备注        |
| :----------: | :-: | :--------------: |
| Screen_Power | 37  |  屏幕供电使能引脚,高电平有效  |
| U2_RS232_Txd | 38  | RS232:  ESP-->屏幕 |
| U2_RS232_Rxd | 39  | RS232:  屏幕-->ESP |

### RS485

|     功能     | 引脚号 |      备注      |
|:------------:|:------:|:--------------:|
|   RS485_DE   |   8    | RS485 RTS引脚  |
| U1_RS485_Txd |   17   | RS485 发送引脚 |
| U1_RS485_Rxd |   18   | RS485 接收引脚 |

### Digital Input Output
#### Digital Input
|    功能    | 引脚号 |          备注           |
| :------: | :-: | :-------------------: |
| DIN_1_IO |  9  | 数字输入 1   外部输入接地，IO低电平 |
| DIN_2_IO |  3  | 数字输入 2   外部输入接地，IO低电平 |
#### Digital Output

|    功能     | 引脚号 |      备注       |
| :-------: | :-: | :-----------: |
| DOUT_1_IO | 12  | 数字输出 1  低电平有效 |
| DOUT_2_IO | 11  | 数字输出 2 低电平有效  |

### 三色灯
|       功能        | 引脚号 |    备注     |
| :-------------: | :-: | :-------: |
| Alarm_Green_IO  |  4  | 绿灯 高电平亮灯  |
| Alarm_Yellow_IO |  5  | 黄灯 高电平亮灯  |
|  Alarm_Red_IO   |  6  | 红灯 高电平亮灯  |
|  Alarm_Beep_IO  |  7  | 蜂鸣器 高电平亮灯 |



# 软件
## IDF SDK配置（通过kconfig进行）
- CONFIG_LWIP_SNTP_MAX_SERVERS  3 设置NTP服务器的个数
- MAIN_TASK_STACK_SIZE 是app_main()task的栈大小 默认为3584，重设为16384
- CONFIG_ETH_SPI_ETHERNET_W5500=y 启用W5500 SPI网卡
- CONFIG_SPIRAM=y 使能PSRAM
- CONFIG_ESPTOOLPY_FLASHSIZE_8MB=y 配置Flash为8M
- CONFIG_PARTITION_TABLE_CUSTOM=y 配置使用自定义分区表，分区表路径在工程根目录

## [程序逻辑](document/程序逻辑.md)

## [MQTT命令表](document/MQTT命令表.md)

# 其他
- [ESP-IDF文档地址](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.2.1/esp32s3/get-started/index.html)
- [ESP-IDF components包管理](https://components.espressif.com/)
- [芯片手册下载地址](https://www.espressif.com/zh-hans/support/documents/technical-documents?keys=&field_type_tid_parent=esp32S3Series-SoCs&field_type_tid%5B%5D=842&field_type_tid_parent=esp32S3Series-Modules&field_type_tid%5B%5D=838)