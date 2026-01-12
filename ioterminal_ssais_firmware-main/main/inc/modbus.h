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
#define MASTER_MAX_CIDS                 1
// Number of reading of parameters from slave
#define MASTER_MAX_RETRY                30
// Timeout to update cid over Modbus
#define UPDATE_CIDS_TIMEOUT_MS          (50)
#define UPDATE_CIDS_TIMEOUT_TICS        (UPDATE_CIDS_TIMEOUT_MS / portTICK_PERIOD_MS)
// Timeout between polls
#define POLL_TIMEOUT_MS                 (50)
#define POLL_TIMEOUT_TICS               (POLL_TIMEOUT_MS / portTICK_PERIOD_MS)
// The macro to get offset for parameter in the appropriate structure
#define HOLD_OFFSET(field)              ((uint16_t)(offsetof(holding_reg_params_t, field) + 1))
#define INPUT_OFFSET(field)             ((uint16_t)(offsetof(input_reg_params_t, field) + 1))
#define COIL_OFFSET(field)              ((uint16_t)(offsetof(coil_reg_params_t, field) + 1))
// Discrete offset macro
#define DISCR_OFFSET(field)             ((uint16_t)(offsetof(discrete_reg_params_t, field) + 1))
#define STR(fieldname)                  ((const char *)(fieldname))
// Options can be used as bit masks or parameter limits
#define OPTS(min_val, max_val, step_val)                   \
    {                                                      \
        .opt1 = min_val, .opt2 = max_val, .opt3 = step_val \
    }

// Enumeration of modbus device addresses accessed by master device
enum
{
    MB_DEVICE_ADDR1 = 1 // Only one slave device used for the test (add other slave addresses here)
};

// Enumeration of all supported CIDs for device (used in parameter definition table)
enum
{
    CID_INP_OPO,
    CID_OUT_MOS
};

// This file defines structure of modbus parameters which reflect correspond modbus address space
// for each modbus register type (coils, discreet inputs, holding registers, input registers)
#pragma pack(push, 1)
typedef struct
{
    uint8_t discrete_input0 : 1;
    uint8_t discrete_input1 : 1;
    uint8_t discrete_input2 : 1;
    uint8_t discrete_input3 : 1;
    uint8_t discrete_input4 : 1;
    uint8_t discrete_input5 : 1;
    uint8_t discrete_input6 : 1;
    uint8_t discrete_input7 : 1;
    uint8_t discrete_input_port1 : 8;
} discrete_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint8_t coils_port0;
    uint8_t coils_port1;
} coil_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint16_t input_data[DIO_MAX_INP];
} input_reg_params_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint16_t output_data[DIO_MAX_OUP];
} holding_reg_params_t;
#pragma pack(pop)

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
    MODBUS_MGR_DATA_REG,
    MODBUS_MGR_CHECK,
    MODBUS_MGR_READY,
    MODBUS_MGR_QUIT
} MbManager_t;

extern void *modbusGetDataRegPtr(const mb_parameter_descriptor_t *param_descriptor);
extern MbManager_t getMbManager();
extern void switchMbManager(MbManager_t mbMgr);
extern void mbManagerLoop();

#endif // _MODBUS_H_
