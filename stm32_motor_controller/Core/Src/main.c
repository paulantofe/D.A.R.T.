/**
 * @file     main.c
 * @brief    Main project source file.
 *
 * @details  This file initializes the configured peripherals and enters the main loop.
 *           The PID control, motor actuation, battery sampling, and encoder processing
 *           runtime is not yet implemented.
 */

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "globals.h"
#include "app.h"

int main(void) {
	STM_Boot();

	if (HAL_UART_Receive_IT(&huart1, &rx_byte, 1) != HAL_OK) {
		Error_Handler();
	}

	while (1) {
		Task_Process_UART_RX();
	}
}


