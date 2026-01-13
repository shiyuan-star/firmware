/**
 * @file modbusTask.c
 * @brief modbus驱动与状态机
 * @version 1.0
 * @date 2024-01-11
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#include "user_tasks.h"
#include "modbus.h"

static char *TAG = "MODBUS";
static mb_parameter_descriptor_t *s_inputParamDescPtr[MB_SLAVE_ADDR_MAX - 1] = {NULL}; // Modbus特征描述符
static mb_parameter_descriptor_t *s_outputParamDescPtr[MB_SLAVE_ADDR_MAX - 1] = {NULL};
static mb_parameter_descriptor_t *s_addrSetParamDescPtr[MB_SLAVE_ADDR_MAX - 1] = {NULL};
static void *s_masterHandlerPtr = NULL;
static MbManager_t s_mbManager = MODBUS_MGR_INIT;
static MbManager_t s_lastMbManager = MODBUS_MGR_INIT;
static RS485ConfigData_t s_rs85ConfigData; // RS485参数配置
QueueHandle_t g_dioInpDataQueueHandler;    // DIO 输入数据接收队列
static uint8_t s_type = 0;

// Example Data (Object) Dictionary for Modbus parameters:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
#define CID_ELEMENTS_NUM 9
const mb_parameter_descriptor_t zsDIOParameters[CID_ELEMENTS_NUM] = {
    // 中盛科技DIO的Modbus描述，参考器件数据手册
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size,
    //  Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    {CID_INP_OPO1, STR("CID_INP_OPO1"), STR("ON/OFF"), MB_SLAVE_ADDR1, MB_PARAM_INPUT, 50, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_READ},
    {CID_OUT_MOS1, STR("CID_OUT_MOS1"), STR("ON/OFF"), MB_SLAVE_ADDR1, MB_PARAM_HOLDING, 53, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_WRITE},
    {CID_INP_OPO2, STR("CID_INP_OPO2"), STR("ON/OFF"), MB_SLAVE_ADDR2, MB_PARAM_INPUT, 50, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_READ},
    {CID_OUT_MOS2, STR("CID_OUT_MOS2"), STR("ON/OFF"), MB_SLAVE_ADDR2, MB_PARAM_HOLDING, 53, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_WRITE},
    {CID_INP_OPO3, STR("CID_INP_OPO3"), STR("ON/OFF"), MB_SLAVE_ADDR3, MB_PARAM_INPUT, 50, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_READ},
    {CID_OUT_MOS3, STR("CID_OUT_MOS3"), STR("ON/OFF"), MB_SLAVE_ADDR3, MB_PARAM_HOLDING, 53, 2,
     0, PARAM_TYPE_U16, 4, OPTS(0, 0, 0), PAR_PERMS_WRITE},
    {CID_ADDR1_SET, STR("CID_ADDR1_SET"), STR("ADDR"), MB_SLAVE_ADDR1, MB_PARAM_HOLDING, 50, 1,
     0, PARAM_TYPE_U8, 1, OPTS(0, 0, 0), PAR_PERMS_WRITE},
    {CID_ADDR2_SET, STR("CID_ADDR2_SET"), STR("ADDR"), MB_SLAVE_ADDR2, MB_PARAM_HOLDING, 50, 1,
     0, PARAM_TYPE_U8, 1, OPTS(0, 0, 0), PAR_PERMS_WRITE},
    {CID_ADDR3_SET, STR("CID_ADDR3_SET"), STR("ADDR"), MB_SLAVE_ADDR3, MB_PARAM_HOLDING, 50, 1,
     0, PARAM_TYPE_U8, 1, OPTS(0, 0, 0), PAR_PERMS_WRITE},
};

/**
 * @brief 获取modbus状态机状态
 * @return MbManager_t
 */
MbManager_t getMbManager()
{
    return s_mbManager;
}

/**
 * @brief 切换modbus状态机事件组
 * @param  mbMgr
 */
void switchMbManager(MbManager_t mbMgr)
{
    // 处在配置DIO状态下,不可操作DIO,不允许进入操作模式
    if (s_mbManager == MODBUS_MGR_SET_DIO_BEGIN && (mbMgr == MODBUS_MGR_READ || mbMgr == MODBUS_MGR_WRITE || mbMgr == MODBUS_MGR_WRITE_OFF))
    {
        return;
    }
    if (mbMgr != s_mbManager)
    {
        s_lastMbManager = s_mbManager;
        s_mbManager = mbMgr;
        // ESP_LOGI(TAG, "MbManager %d -> %d", s_lastMbManager, s_mbManager);
    }
}

/**
 * @brief Modbus状态机初始化
 * @param  rs485ConfigData
 */
void mbMgrInit(RS485ConfigData_t rs485ConfigData)
{
    s_rs85ConfigData = rs485ConfigData;
    if (getMbManager() != MODBUS_MGR_INIT)
    {
        switchMbManager(MODBUS_MGR_QUIT);
        while (s_lastMbManager != s_mbManager)
        {
            mbManagerLoop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    switchMbManager(MODBUS_MGR_INIT);
}

/**
 * @brief modbus状态机循环
 */
void mbManagerLoop()
{
    esp_err_t err;

    switch (s_mbManager)
    {
    case MODBUS_MGR_INIT:
    {
        ESP_LOGW(TAG, "Modbus started");
        err = mbc_master_init(MB_PORT_SERIAL_MASTER, &s_masterHandlerPtr);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller init failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_SETUP);
        }
        break;
    }

    case MODBUS_MGR_SETUP:
    {
        // Initialize and start Modbus controller
        mb_communication_info_t comm = {.port = MB_PORT_NUM,
                                        .mode = MB_MODE_RTU,
                                        .baudrate = s_rs85ConfigData.baudrate,
                                        .parity = MB_PARITY_NONE};
        err = mbc_master_setup((void *)&comm);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller setup failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_PIN);
        }
        break;
    }

    case MODBUS_MGR_PIN:
    {
        // Set UART pin numbers
        err = uart_set_pin(MB_PORT_NUM, MODBUS_UART_TX_PIN, MODBUS_UART_RX_PIN, MODBUS_RS485_DE_PIN, UART_PIN_NO_CHANGE);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller set pin failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_START);
        }
        break;
    }

    case MODBUS_MGR_START:
    {
        err = mbc_master_start();
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller start failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_MODE);
        }
        break;
    }

    case MODBUS_MGR_MODE:
    {
        // Set driver mode to Half Duplex
        err = uart_set_mode(MB_PORT_NUM, UART_MODE_RS485_HALF_DUPLEX);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller set mode failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_DESC);
        }
        break;
    }

    case MODBUS_MGR_DESC:
    {
        if (s_rs85ConfigData.slaveModel == ZS_DIO)
        {
            err = mbc_master_set_descriptor(&zsDIOParameters[0], CID_ELEMENTS_NUM);
            if (err != ESP_OK)
            {
                ESP_LOGI(TAG, "Modbus controller set descriptor failed, error=0x%x", err);
            }
            else
            {
                switchMbManager(MODBUS_MGR_INP_OPO);
            }
        }
        break;
    }

    case MODBUS_MGR_INP_OPO:
    {
        // 获取光耦输入的特征描述符
        bool getCidInfoFail = false;
        for (size_t i = 0; i < s_rs85ConfigData.slaveNum; i++)
        {
            err = mbc_master_get_cid_info(i * 2, &s_inputParamDescPtr[i]);
            if ((err != ESP_OK) && (s_inputParamDescPtr[i] == NULL))
            {
                getCidInfoFail = true;
                ESP_LOGE(TAG, "Failed to get CID_INP_OPO%d parameters descriptor.", i + 1);
            }
        }
        if (!getCidInfoFail)
        {
            switchMbManager(MODBUS_MGR_OUT_MOS);
        }
        break;
    }

    case MODBUS_MGR_OUT_MOS:
    {
        // 获取MOS输出的特征描述符
        bool getCidInfoFail = false;
        for (size_t i = 0; i < s_rs85ConfigData.slaveNum; i++)
        {
            err = mbc_master_get_cid_info((i * 2) + 1, &s_outputParamDescPtr[i]);
            if ((err != ESP_OK) && (s_outputParamDescPtr[i] == NULL))
            {
                getCidInfoFail = true;
                ESP_LOGE(TAG, "Failed to get CID_OUT_MOS%d parameters descriptor.", i + 1);
            }
        }
        if (!getCidInfoFail)
        {
            switchMbManager(MODBUS_MGR_ADDR_SET);
        }
        break;
    }

    case MODBUS_MGR_ADDR_SET:
    {
        // 获取地址设置的特征描述符
        bool getCidInfoFail = false;
        for (size_t i = 0; i < MB_SLAVE_ADDR_MAX - 1; i++)
        {
            err = mbc_master_get_cid_info(i + CID_ADDR1_SET, &s_addrSetParamDescPtr[i]);
            if ((err != ESP_OK) && (s_addrSetParamDescPtr[i] == NULL))
            {
                getCidInfoFail = true;
                ESP_LOGE(TAG, "Failed to get CID_ADDR%d_SET parameters descriptor.", i + 1);
            }
        }
        if (!getCidInfoFail)
        {
            switchMbManager(MODBUS_MGR_WRITE_OFF);
        }
        break;
    }

    case MODBUS_MGR_WRITE:
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
    }

    case MODBUS_MGR_WRITE_OFF:
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
    }

    case MODBUS_MGR_READ:
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
    }

    case MODBUS_MGR_SET_DIO_BEGIN:
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        break;
    }

    case MODBUS_MGR_SET_DIO_FINISH: // 退出配置DIO地址模式,返回读取模式
    {
        switchMbManager(MODBUS_MGR_READ);
        break;
    }

    case MODBUS_MGR_QUIT:
    {
        if (s_lastMbManager != s_mbManager)
        {
            if (s_mbManager >= MODBUS_MGR_START)
            {
                mbc_master_destroy();
            }
            ESP_LOGI(TAG, "Modbus stoped");
            s_lastMbManager = s_mbManager;
        }
        break;
    }
    }
}

/**
 * @brief  按位比较端口值，发送到队列中
 * @param  now 现在端口的值
 * @param  pre 前一个端口的值
 * @param  portOffset 端口偏移量
 */
void compareBitsSendToQueue(uint32_t now, uint32_t pre, uint8_t portOffset)
{
    DioInputData_t dioInputData = {0};
    // 按位比较端口数据
    for (int i = 0; i < sizeof(uint32_t) * 8; i++)
    {
        if ((now & (1 << i)) != (pre & (1 << i)))
        {
            // ESP_LOGI(TAG, "Bit %d: Now=%d, Pre=%d", i, (now & (1 << i)) ? 1 : 0, (pre & (1 << i)) ? 1 : 0);
            dioInputData.vaule = (now & (1 << i)) ? 1 : 0;
            dioInputData.inputPort = (portOffset * DIO_PORT_NUM_MAX) + i + 1; // 偏移量 + 端口变化位 + 1
            xQueueSend(g_dioInpDataQueueHandler, &dioInputData, 0);
        }
    }
}

/**
 * @brief 控制DIO输出
 * @param  addr
 * @param  turnOn
 */
void setDioOnOff(uint8_t addr, bool turnOn)
{
    uint8_t type = 0;
    uint32_t tunOnVaule = 0xFFFFFFFF;
    uint32_t tunOffVaule = 0;
    if (addr > s_rs85ConfigData.slaveNum) // 超过设置的地址限制，不执行
    {
        return;
    }

    switch (addr)
    {
    case MB_SLAVE_ADDR1:
        if (turnOn)
        {
            mbc_master_set_parameter(1, (char *)s_outputParamDescPtr[0]->param_key, (uint8_t *)&tunOnVaule, &type);
        }
        else
        {
            mbc_master_set_parameter(1, (char *)s_outputParamDescPtr[0]->param_key, (uint8_t *)&tunOffVaule, &type);
        }
        break;
    case MB_SLAVE_ADDR2:
        if (turnOn)
        {
            mbc_master_set_parameter(3, (char *)s_outputParamDescPtr[1]->param_key, (uint8_t *)&tunOnVaule, &type);
        }
        else
        {
            mbc_master_set_parameter(3, (char *)s_outputParamDescPtr[1]->param_key, (uint8_t *)&tunOffVaule, &type);
        }
        break;
    case MB_SLAVE_ADDR3:
        if (turnOn)
        {
            mbc_master_set_parameter(5, (char *)s_outputParamDescPtr[2]->param_key, (uint8_t *)&tunOnVaule, &type);
        }
        else
        {
            mbc_master_set_parameter(5, (char *)s_outputParamDescPtr[2]->param_key, (uint8_t *)&tunOffVaule, &type);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief  设置DIO外设RS485通信地址
 * @param  nowSendAddr
 * @param  writeAddr
 * @return esp_err_t
 */
esp_err_t setDioAddr(uint8_t nowSendAddr, uint8_t writeAddr)
{
    uint8_t type = 0;
    esp_err_t err;
    switch (nowSendAddr)
    {
    case MB_SLAVE_ADDR1:
        err = mbc_master_set_parameter(CID_ADDR1_SET, (char *)s_addrSetParamDescPtr[0]->param_key, &writeAddr, &type);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Modbus controller set CID_ADDR1_SET to addr[%d] Failed", writeAddr);
            return err;
        }
        return ESP_OK;
        break;
    case MB_SLAVE_ADDR2:
        err = mbc_master_set_parameter(CID_ADDR2_SET, (char *)s_addrSetParamDescPtr[1]->param_key, &writeAddr, &type);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Modbus controller set CID_ADDR2_SET to addr[%d] Failed", writeAddr);
            return err;
        }
        return ESP_OK;
        break;
    case MB_SLAVE_ADDR3:
        err = mbc_master_set_parameter(CID_ADDR3_SET, (char *)s_addrSetParamDescPtr[2]->param_key, &writeAddr, &type);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Modbus controller set CID_ADDR3_SET to addr[%d] Failed", writeAddr);
            return err;
        }
        return ESP_OK;
        break;

    default:
        ESP_LOGE(TAG, "The current communication address exceeds the limit ");
        return ESP_ERR_INVALID_ARG;
        break;
    }
}

/**
 * @brief  查询在线的DIO
 * @return uint8_t 返回在线设备按位掩码
 */
uint8_t searchForOnlineDevices()
{
    uint8_t onlineDevicesMask = 0x00;
    esp_err_t err;
    uint32_t inputData;
    for (size_t i = 0; i < MB_SLAVE_ADDR_MAX - 1; i++)
    {
        err = mbc_master_get_parameter(i * 2, (char *)s_inputParamDescPtr[i]->param_key, (uint8_t *)&inputData, &s_type);
        if (err != ESP_OK)
        {
            ESP_LOGW(TAG, "DIO ADDR[%d] Offline", i + 1);
        }
        else
        {
            onlineDevicesMask = onlineDevicesMask | (0x01 << i);
            ESP_LOGI(TAG, "DIO ADDR[%d] Online", i + 1);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    return onlineDevicesMask;
}

/**
 * @brief modbusTask
 * @param  pvParameters
 */
void modbusTask(void *pvParameters)
{
    esp_err_t err = ESP_OK;
    mbMgrInit(g_nvsData.DeviceConfigData.rs485ConfigData);
    uint32_t input32DIOPre[MB_SLAVE_ADDR_MAX - 1] = {0};
    uint32_t input32DIONow[MB_SLAVE_ADDR_MAX - 1] = {0};

    for (;;)
    {
        mbManagerLoop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}