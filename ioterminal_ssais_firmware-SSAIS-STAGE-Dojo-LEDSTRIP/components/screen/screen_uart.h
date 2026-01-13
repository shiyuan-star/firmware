/**
 * @file screen_uart.h
 * @brief 大彩串口屏ESP32S3串口驱动
 * @author ChenJinBo (VP01130@globalymc.com)
 * @version 1.0
 * @date 2023-12-18
 * 
 * @copyright Copyright (c) 2023  雅马哈发动机（厦门）信息系统有限公司
 * 
 */
#ifndef _SCREEN_UART_H
#define _SCREEN_UART_H

#include <stdio.h>


void screenInit(int baud,int txPin,int rxPix);
void sendChar(uint8_t t);

#endif //_SCREEN_UART_H
