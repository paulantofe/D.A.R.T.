#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

/**
 * @brief Handle errors by freezing the board and disabling interrupts.
 */
void Error_Handler(void);

/**
 * @brief   Configure system clock to maximum speed.
 *
 * @details Establishes the core and peripheral bus maximum frequencies:
 *            - SYSCLK / HCLK : 168 MHz
 *            - APB1 (PCLK1)  :  42 MHz
 *            - APB2 (PCLK2)  :  84 MHz
 */
void SystemClock_Config(void);

/**
 * @brief   Configure the USART1 Peripheral.
 *
 * @details The UART Protocol is used to transmit/receive messages to/from esp32 (Master).
 */
void UART_Config(void);

/**
 * @brief   Configure the TIM6 Peripheral
 *
 * @details Generate a 10ms (100Hz) time-base interrupt for the PID control loop.
 */
void TIM6_Config(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
