#ifndef INC_STM32F4XX_HAL_HARDWARE_CONFIG_H_
#define INC_STM32F4XX_HAL_HARDWARE_CONFIG_H_

#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
extern ADC_HandleTypeDef hadc1;

/**
 * @brief   Boot the STM32F407G-DISC1 Board.
 * @details Initialize the necessary peripherals and trigger System_Fault state upon fail.
 */
void STM_Boot(void);

/**
 * @brief   System Fault State.
 * @details Freeze the system, disable all interrupts and blink the Red LED.
 * @note    Software blocking delay is used because SysTick/HAL_Delay might be broken.
 */
void Error_Handler(void);

/**
 * @brief   Configure system clock to maximum speed.
 * @details Establishes the core and peripheral bus maximum frequencies:
 *            - SYSCLK / HCLK : 168 MHz
 *            - APB1 (PCLK1)  :  42 MHz
 *            - APB2 (PCLK2)  :  84 MHz
 */
void SystemClock_Config(void);

/**
 * @brief   Configure the on-board Status LEDs.
 * @details Red LED indicates System_Fault state
 *          Orange LED indicates Fail_Safe State
 *          Instance: GPIOD | PD13 (Orange) | PD14 (Red)
 */
void LED_Config(void);

/**
 * @brief   Configure the USART1 Peripheral.
 * @details The UART Protocol is used to transmit/receive messages to/from esp32 (Master).
 *          Instance: USART1 | TX: PA9 | RX: PA10 | Baud: 115200.
 */
void UART_Config(void);

/**
 * @brief   Configure the TIM6 Peripheral.
 * @details Generate a 10ms (100Hz) time-base interrupt for the PID control loop.
 *          Instance: TIM6.
 */
void TIM6_Config(void);

/**
 * @brief   Configure the TIM3 Peripheral.
 * @details Generate a 20kHz PWM signal (4200 steps resolution) on
 *          Channel 1 (Left Motor) and Channel 2 (Right Motor).
 *          Instance: TIM3 | CH1: PA6 | CH2: PA7
 */
void TIM3_Config(void);

/**
 * @brief   Configure the TIM2 Peripheral.
 * @details Set up TIM2 (32-bit) in Hardware Encoder Mode (X4 resolution)
 *          to track position and speed for the Left Motor.
 */
void TIM2_Config(void);

/**
 * @brief   Configure the TIM5 Peripheral.
 * @details Set up TIM5 (32-bit) in Hardware Encoder Mode (X4 resolution)
 *          to track position and speed for the Right Motor.
 */
void TIM5_Config(void);

/**
 * @brief   Configure the ADC1 Peripheral.
 * @details The ADC1 peripheral is configured to read battery voltage
 */
void ADC1_Config(void);

#endif
