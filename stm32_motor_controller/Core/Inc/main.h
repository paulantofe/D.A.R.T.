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
 * @brief   Configure the on-board Status LEDs.
 *
 * @details Red LED indicates System_Fault state
 *          Orange LED indicates Fail_Safe State
 *          Instance: GPIOD | PD13 (Orange) | PD14 (Red)
 */
void LED_Config(void);

/**
 * @brief   Configure the USART1 Peripheral.
 *
 * @details The UART Protocol is used to transmit/receive messages to/from esp32 (Master).
 *          Instance: USART1 | TX: PA9 | RX: PA10 | Baud: 115200.
 */
void UART_Config(void);

/**
 * @brief   Configure the TIM6 Peripheral.
 *
 * @details Generate a 10ms (100Hz) time-base interrupt for the PID control loop.
 *          Instance: TIM6.
 */
void TIM6_Config(void);

/**
 * @brief   Configure the TIM3 Peripheral.
 *
 * @details Generate a 20kHz PWM signal (4200 steps resolution) on
 *          Channel 1 (Left Motor) and Channel 2 (Right Motor).
 *          Instance: TIM3 | CH1: PA6 | CH2: PA7
 */
void TIM3_Config(void);

#ifdef __cplusplus
}
#endif

#endif
