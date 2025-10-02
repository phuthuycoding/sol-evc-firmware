/**
 * @file main.h
 * @brief Main header file for STM32F103 Master firmware
 * @version 1.0.0
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

/* Shared includes */
#include "uart_protocol.h"
#include "ocpp_messages.h"
#include "device_config.h"

/* Exported types */
#define UNUSED(x) ((void)(x))

/* Hardware handles */
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart1;  // ESP8266
extern UART_HandleTypeDef huart2;  // Debug
extern UART_HandleTypeDef huart3;  // RS485
extern SPI_HandleTypeDef hspi1;    // CS5460A

/* Function prototypes */
void Error_Handler(void);
void SystemClock_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
