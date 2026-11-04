/**
 * @file     main.c
 * @brief    Main project source file.
 *
 * @details  This file initializes used peripherals. If any of these fails, it triggers System_Fault state.
 *           It also acts as a PID regulator, generates PWM signal for motors, reads battery voltage level
 *           and the motor encoders.
 */

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"

int main(void) {
	STM_Boot();

	while (1) {

	}
}



