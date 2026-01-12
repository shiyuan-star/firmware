/**
 * @file board.h
 * @brief 二值化设备输出效果定义
 * @version 1.0
 * @date 2023-12-15
 *
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 *
 */
#ifndef _BOARD_H_
#define _BOARD_H_

// DIO
#define DIO_MAX_INP 32           // 系统支持DIO最大输入路数
#define DIO_MAX_OUP 32           // 系统支持DIO最大输出路数
#define DIO_RECEIVE_QUEUE_LEN 10 // 只记录10次动作变化
// UART1
#define MB_PORT_NUM UART_NUM_1
#define MB_DEV_SPEED 38400
#define MODBUS_UART_EVEN_QUEUE_SIZE 20
#define MODBUS_UART_DATA_QUEUE_SIZE 10
#define MODBUS_UART_TX_PIN 17
#define MODBUS_UART_RX_PIN 18
#define MODBUS_RS485_DE_PIN 8
#define MODBUS_UART_BUF_SIZE (1024)
#define MODBUS_RD_BUF_SIZE (DAM_UART_BUF_SIZE)

/**
 * @brief 板载灯效类型定义
 */
typedef enum
{
    STATE_LED_BOOT_BLINK,
    STATE_LED_FAST_BLINK_LOOP,
    STATE_LED_SLOW_BLINK_LOOP,
    STATE_LED_BREATHE,
    STATE_LED_MAX,
} state_led_effect_type_t;

/**
 * @brief 警示灯灯效类型定义
 */
typedef enum
{
    ALARM_LED_FAST_DOUBLE_ON_OFF,
    ALARM_LED_TRIPLE_ON_OFF,
    ALARM_LED_TWO_SHORT_ONE_LONG_LOOP,
    ALARM_LED_FAST_BLINK_LOOP,
    ALARM_LED_SLOW_BLINK_LOOP,
    ALARM_LED_MAX
} alarm_led_effect_type_t;

typedef enum
{
    DIGITAL_OUTPUT_SLOW_BLINK_LOOP,
    DIGITAL_OUTPUT_MAX,
} digital_output_effect_type_t;

#endif // _BOARD_H_
