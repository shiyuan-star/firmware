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
static holding_reg_params_t s_holding_reg_params = {0};
static input_reg_params_t s_input_reg_params = {0};
static coil_reg_params_t s_coil_reg_params = {0};
static discrete_reg_params_t s_discrete_reg_params = {0};
static mb_parameter_descriptor_t *inputParamDescPtr = NULL; // Modbus特征描述符
static mb_parameter_descriptor_t *outputParamDescPtr = NULL;
static uint32_t s_nowLampLedVaule = 0x00000000; // 当前指示灯值
static uint32_t s_nowSensorVaule = 0x00000000;  // 当前传感器状态
static void *masterHandlerPtr = NULL;
static void *inputDataPtr = NULL;
static MbManager_t s_mbManager = MODBUS_MGR_INIT;
static MbManager_t s_lastMbManager = MODBUS_MGR_INIT;
QueueHandle_t g_dioInpDataQueueHandler; // DIO 输入数据接收队列

// Example Data (Object) Dictionary for Modbus parameters:
// The CID field in the table must be unique.
// Modbus Slave Addr field defines slave address of the device with correspond parameter.
// Modbus Reg Type - Type of Modbus register area (Holding register, Input Register and such).
// Reg Start field defines the start Modbus register number and Reg Size defines the number of registers for the characteristic accordingly.
// The Instance Offset defines offset in the appropriate parameter structure that will be used as instance to save parameter value.
// Data Type, Data Size specify type of the characteristic and its data size.
// Parameter Options field specifies the options that can be used to process parameter value (limits or masks).
// Access Mode - can be used to implement custom options for processing of characteristic (Read/Write restrictions, factory mode values and etc).
const mb_parameter_descriptor_t deviceParameters[] = {
    // { CID, Param Name, Units, Modbus Slave Addr, Modbus Reg Type, Reg Start, Reg Size,
    //  Instance Offset, Data Type, Data Size, Parameter Options, Access Mode}
    {CID_INP_OPO, STR("CID_INP_OPO"), STR("ON/OFF"), MB_DEVICE_ADDR1, MB_PARAM_INPUT, 0, DIO_MAX_INP,
     INPUT_OFFSET(input_data), PARAM_TYPE_U16, PARAM_SIZE_U16, OPTS(0x00, 0x01, 1), PAR_PERMS_READ},
    {CID_OUT_MOS, STR("CID_OUT_MOS"), STR("ON/OFF"), MB_DEVICE_ADDR1, MB_PARAM_HOLDING, 0, DIO_MAX_OUP,
     HOLD_OFFSET(output_data), PARAM_TYPE_U16, 2, OPTS(0, 100, 1), PAR_PERMS_WRITE}};
// Calculate number of parameters in the table

const uint16_t numDeviceParameters = (sizeof(deviceParameters) / sizeof(deviceParameters[0]));

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
    if (mbMgr != s_mbManager)
    {
        s_lastMbManager = s_mbManager;
        s_mbManager = mbMgr;
        ESP_LOGI(TAG, "MbManager %d -> %d", s_lastMbManager, s_mbManager);
    }
}

/**
 * @brief The function to get pointer to parameter storage (instance) according to parameter description table
 * @param  param_descriptor
 * @return void*
 */
void *modbusGetDataRegPtr(const mb_parameter_descriptor_t *param_descriptor)
{
    assert(param_descriptor != NULL);
    void *instancePtr = NULL;
    if (param_descriptor->param_offset != 0)
    {
        switch (param_descriptor->mb_param_type)
        {
        case MB_PARAM_HOLDING:
            instancePtr = ((void *)&s_holding_reg_params + param_descriptor->param_offset - 1);
            break;
        case MB_PARAM_INPUT:
            instancePtr = ((void *)&s_input_reg_params + param_descriptor->param_offset - 1);
            break;
        case MB_PARAM_COIL:
            instancePtr = ((void *)&s_coil_reg_params + param_descriptor->param_offset - 1);
            break;
        case MB_PARAM_DISCRETE:
            instancePtr = ((void *)&s_discrete_reg_params + param_descriptor->param_offset - 1);
            break;
        default:
            instancePtr = NULL;
            break;
        }
    }
    else
    {
        ESP_LOGE(TAG, "Wrong parameter offset for CID #%d", param_descriptor->cid);
    }
    return instancePtr;
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
        ESP_LOGI(TAG, "Modbus started");
        inputParamDescPtr = NULL;
        outputParamDescPtr = NULL;
        err = mbc_master_init(MB_PORT_SERIAL_MASTER, &masterHandlerPtr);
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
                                        .baudrate = MB_DEV_SPEED,
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
        err = mbc_master_set_descriptor(&deviceParameters[0], numDeviceParameters);
        if (err != ESP_OK)
        {
            ESP_LOGI(TAG, "Modbus controller set descriptor failed, error=0x%x", err);
        }
        else
        {
            switchMbManager(MODBUS_MGR_INP_OPO);
        }
        break;
    }

    case MODBUS_MGR_INP_OPO:
    {
        // 获取光耦输入的特征描述符
        err = mbc_master_get_cid_info(CID_INP_OPO, &inputParamDescPtr);
        if ((err == ESP_ERR_NOT_FOUND) && (inputParamDescPtr == NULL))
        {
            ESP_LOGE(TAG, "Failed to get CID_INP_OPO parameters descriptor.");
        }
        else
        {
            switchMbManager(MODBUS_MGR_OUT_MOS);
        }
        break;
    }

    case MODBUS_MGR_OUT_MOS:
    {
        // 获取MOS输出的特征描述符
        err = mbc_master_get_cid_info(CID_OUT_MOS, &outputParamDescPtr);
        if ((err == ESP_ERR_NOT_FOUND) && (outputParamDescPtr == NULL))
        {
            ESP_LOGE(TAG, "Failed to get CID_OUT_MOS parameters descriptor.");
        }
        else
        {
            switchMbManager(MODBUS_MGR_DATA_REG);
        }
        break;
    }

    case MODBUS_MGR_DATA_REG:
    {
        // 输入寄存器指针地址
        inputDataPtr = modbusGetDataRegPtr(inputParamDescPtr);
        if (inputDataPtr == NULL)
        {
            ESP_LOGE(TAG, "Failed to get DataRegPtr.");
        }
        else
        {
            switchMbManager(MODBUS_MGR_CHECK);
        }
        break;
    }

    case MODBUS_MGR_CHECK:
    {
        break;
    }

    case MODBUS_MGR_READY:
    {
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
 * @brief modbusTask
 * @param  pvParameters
 */
void modbusTask(void *pvParameters)
{
    esp_err_t err = ESP_OK;
    uint16_t nowInputREG[DIO_MAX_INP] = {0}; // DIO光耦输入数据
    uint16_t preInputREG[DIO_MAX_INP] = {0};
    uint16_t outputREG[DIO_MAX_INP] = {0}; // MOS管输出数据
    uint8_t type = 0;
    DioInputData_t dioInputData;

    for (;;)
    {
        mbManagerLoop();
        if (getMbManager() == MODBUS_MGR_CHECK)
        {
            err = mbc_master_get_parameter(CID_INP_OPO, (char *)inputParamDescPtr->param_key, (uint8_t *)&nowInputREG[0], &type);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Modbus controller get CID_INP_OPO parameter failed, error=0x%x", err);
            }
            else
            {
                memcpy(inputDataPtr, nowInputREG, (2 * inputParamDescPtr->mb_size));
                for (uint8_t i = 0; i < DIO_MAX_INP; i++)
                {
                    if (nowInputREG[i])
                    {
                        s_nowSensorVaule = s_nowSensorVaule | (0x1 << i);
                    }
                    else
                    {
                        s_nowSensorVaule = s_nowSensorVaule & (~(0x1 << i));
                    }
                }
                switchMbManager(MODBUS_MGR_READY);
            }
        }
        if (getMbManager() == MODBUS_MGR_READY)
        {
            err = mbc_master_get_parameter(CID_INP_OPO, (char *)inputParamDescPtr->param_key, (uint8_t *)&nowInputREG[0], &type);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Modbus controller get CID_INP_OPO parameter failed, error=0x%x", err);
            }
            else
            {
                memcpy(inputDataPtr, nowInputREG, (2 * inputParamDescPtr->mb_size));
                for (uint8_t i = 0; i < DIO_MAX_INP; i++)
                {
                    if (nowInputREG[i] != preInputREG[i]) // 与前一个状态数据不一致
                    {
                        dioInputData.inputPort = i + 1; // Todo 输入重映射
                        dioInputData.vaule = nowInputREG[i];
                        dioInputData.time = esp_log_timestamp();
                        if (nowInputREG[i])
                        {
                            s_nowSensorVaule = s_nowSensorVaule | (0x1 << i);
                        }
                        else
                        {
                            s_nowSensorVaule = s_nowSensorVaule & (~(0x1 << i));
                        }
                        xQueueSend(g_dioInpDataQueueHandler, &dioInputData, pdMS_TO_TICKS(10));
                        preInputREG[i] = nowInputREG[i];
                    }
                }
            }
            for (uint8_t j = 0; j < DIO_MAX_OUP; j++) // Todo 输出重映射
            {
                if (s_nowLampLedVaule & (1 << j))
                {
                    outputREG[j] = 0x0001;
                }
                else
                {
                    outputREG[j] = 0x0000;
                }
            }
            err = mbc_master_set_parameter(CID_OUT_MOS, (char *)outputParamDescPtr->param_key, (uint8_t *)outputREG, &type);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Modbus controller set CID_OUT_MOS parameter failed, error=0x%x", err);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelete(NULL);
}