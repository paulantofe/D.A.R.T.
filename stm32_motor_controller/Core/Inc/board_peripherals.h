/**
 * @file    board_peripherals.h
 * @brief   Shared STM32 board peripheral handles.
 * @details Declares the peripheral handles owned by the board hardware
 *          configuration module.
 */

#ifndef INC_BOARD_PERIPHERALS_H_
#define INC_BOARD_PERIPHERALS_H_

#include "stm32f4xx_hal.h"

/* -------------- Board Peripheral Handles -------------- */

extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
extern DMA_HandleTypeDef hdma1;
extern I2C_HandleTypeDef hi2c2;

/* ------------------------------------------------------ */

#endif /* INC_BOARD_PERIPHERALS_H_ */
