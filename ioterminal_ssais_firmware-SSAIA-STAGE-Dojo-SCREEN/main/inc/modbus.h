/**
 * @file modbus.h
 * @brief modbus头文件
 * @version 1.0
 * @date 2024-01-11
 *
 * @copyright Copyright (c) 2024  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _MODBUS_H_
#define _MODBUS_H_

#include "common.h"
// // The number of parameters that intended to be used in the particular control process
#define STR(fieldname) ((const char *)(fieldname))
#define DIO_PORT_NUM_MAX 32                       // 使用的DIO端口数 （中盛科技的16口和32口寄存器地址通用，故32口包含16口）
#define BUTTON_LED_INDICATION_LED_BLINK_CYCLE 400 // 指示灯定时器周期(MS)
#define BUTTON_LED_ON_OFF_SWITCHING_RATIO 2       // 指示灯开和关的时间比例

// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val) \
    {                                    \
        .opt1 = min_val, .opt2 = max_val, .opt3 = step_val}

// Enumeration of modbus device addresses accessed by master device
enum
{
    MB_SLAVE_ADDR1 = 1,
    MB_SLAVE_ADDR2,
    MB_SLAVE_ADDR3,
    MB_SLAVE_ADDR_MAX
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum
{
    CID_INP_OPO1,
    CID_OUT_MOS1,
    CID_INP_OPO2,
    CID_OUT_MOS2,
    CID_INP_OPO3,
    CID_OUT_MOS3,
    CID_ADDR1_SET,
    CID_ADDR2_SET,
    CID_ADDR3_SET,
};

typedef enum
{
    ZS_DIO,             /*!< Device model: ZS_DIO */
    RS485_MODEL_INVALID /*!< Invalid RS485 model */
} RS485_model_t;

typedef enum
{
    MODBUS_MGR_INIT = 0x00,
    MODBUS_MGR_SETUP,
    MODBUS_MGR_PIN,
    MODBUS_MGR_START,
    MODBUS_MGR_MODE,
    MODBUS_MGR_DESC,
    MODBUS_MGR_INP_OPO,
    MODBUS_MGR_OUT_MOS,
    MODBUS_MGR_ADDR_SET,
    MODBUS_MGR_WRITE,           // 根据输出变量写DIO
    MODBUS_MGR_WRITE_OFF,       // DIO全部关闭
    MODBUS_MGR_READ,            // 读取DIO输入
    MODBUS_MGR_SET_DIO_BEGIN,   // 进入配置DIO地址模式
    MODBUS_MGR_SET_DIO_FINISH,  // 退出配置DIO地址模式
    MODBUS_MGR_QUIT
} MbManager_t;

extern void *modbusGetDataRegPtr(const mb_parameter_descriptor_t *param_descriptor);
extern MbManager_t getMbManager();
extern esp_err_t setDioAddr(uint8_t nowSendAddr, uint8_t writeAddr);
extern void setDioOnOff(uint8_t addr, bool turnOn);
extern uint8_t searchForOnlineDevices();
extern void switchMbManager(MbManager_t mbMgr);
extern void mbManagerLoop();

#endif // _MODBUS_H_
