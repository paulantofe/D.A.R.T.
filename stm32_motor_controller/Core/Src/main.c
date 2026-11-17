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

/* ------------------ Shared Application State ------------------ */

/** Current high-level application state. */
volatile System_State_t system_state = Offline;

/** Current ESP32-to-STM32 RDT link state. */
volatile Link_State_t link_state = LINK_OFFLINE;

volatile uint8_t received_byte = 0;

volatile uint8_t current_command = RDT_CONTROL_STOP;

/* -------------------------------------------------------------- */


int main(void) {
	STM_Boot();

	while (1) {
		RDT_Link_CheckTimeout(HAL_GetTick());
	}
}


