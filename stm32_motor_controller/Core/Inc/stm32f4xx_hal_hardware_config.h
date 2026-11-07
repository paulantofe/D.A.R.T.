/**
 * @file     stm32f4xx_hal_hardware_config.h
 * @brief    Hardware Configuration header file.
 *
 * @details  This file declares the high-level peripheral configuration APIs.
 */

#ifndef INC_STM32F4XX_HAL_HARDWARE_CONFIG_H_
#define INC_STM32F4XX_HAL_HARDWARE_CONFIG_H_

#include "stm32f4xx_hal.h"

/* -------------- Peripheral Handle Structures -------------- */
// Note: Any file including this header file will have access to the
//       handle structures (needed for HAL APIs).

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
extern ADC_HandleTypeDef hadc1;

/* ---------------------------------------------------------- */


/* -------------- Pin Definition Macros -------------- */

#define GREEN_LED       GPIO_PIN_12
#define ORANGE_LED      GPIO_PIN_13
#define RED_LED         GPIO_PIN_14
#define BLUE_LED        GPIO_PIN_15

/* --------------------------------------------------- */


/**
 * @defgroup FS_Trigger        Failsafe state trigger event
 * @{
 */
typedef enum {
	ESP_Sent_F,
	ESP_Link_Lost
} FS_Trigger_t;
/** @} */


/* -------------- Hardware Configuration APIs -------------- */

/**
 * @brief   Boot the STM32F407G-DISC1 Board.
 * @details Initialize the necessary peripherals and trigger System_Fault state upon fail.
 *          Wait for ESP32 to boot before going into thread mode.
 */
void STM_Boot(void);

/**
 * @brief   System Fault State.
 * @details Freeze the system, disable all interrupts and blink the Red LED.
 * @note    Software blocking delay is used because SysTick/HAL_Delay might be broken.
 */
void Error_Handler(void);

/**
 * @brief Failsafe state.
 */
void Failsafe_Handler();

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
 * @details Green LED indicates that the system is up (ESP and STM boot successfully).
 *          Blue LED indicates that STM32 is ready and waiting for ESP32 to boot (sends OK - "K").
 *          Red LED indicates System_Fault state (STM32 / ESP32).
 *          Orange LED indicates Fail_Safe State.
 *          Instance: GPIOD | PD12 (Green) | PD13 (Orange) | PD14 (Red) | PD15 (Blue)
 */
void LED_Config(void);

/**
 * @brief   Configure the USART1 Peripheral.
 * @details The UART Protocol is used to transmit/receive messages to/from esp32 (Master).
 *          Instance: USART1 | TX: PB6 | RX: PB7 | Baud: 115200.
 */
void UART_Config(void);

/**
 * @brief   Configure the TIM6 Peripheral.
 * @details Configure a 10ms (100Hz) time base intended for the PID control loop.
 *          Instance: TIM6.
 */
void TIM6_Config(void);

/**
 * @brief   Configure the TIM3 Peripheral.
 * @details Configure PWM for a 20kHz signal (4200 steps resolution) on
 *          Channel 1 (Left Motor) and Channel 2 (Right Motor).
 *          Instance: TIM3 | CH1: PA6 | CH2: PA7
 */
void TIM3_Config(void);

/**
 * @brief   Configure the TIM2 Peripheral.
 * @details Configure TIM2 (32-bit) in hardware encoder mode (X4 resolution)
 *          for the Left Motor position and speed feedback.
 *          Instance: TIM2 | CH1: PA15 | CH2: PB3
 */
void TIM2_Config(void);

/**
 * @brief   Configure the TIM5 Peripheral.
 * @details Configure TIM5 (32-bit) in hardware encoder mode (X4 resolution)
 *          for the Right Motor position and speed feedback.
 *          Instance: TIM5 | CH1: PA0 | CH2: PA1
 */
void TIM5_Config(void);

/**
 * @brief   Configure the ADC1 Peripheral.
 * @details Configure ADC1 for battery-voltage sampling.
 *          Instance: ADC1 | CH3: PA3 */
void ADC1_Config(void);

/* --------------------------------------------------------- */

#endif
