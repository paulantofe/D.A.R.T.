/**
 * @file     app.c
 * @brief    STM32 application state and UART transmission logic.
 */

#include "app.h"
#include "globals.h"
#include "rdt_link.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"
#include <stdbool.h>
#include <stdint.h>
#include <rdt_protocol.h>

/* ------------------ Shared Application State ------------------ */

/** True until the first valid ESP32 handshake response is received. */
volatile bool boot_handshake = true;

/* -------------------------------------------------------------- */


/* ------------------ Failsafe State APIs ------------------ */

/**
 * @brief Enter the runtime failsafe state.
 */
void Failsafe_Enter(void) {
	if (boot_handshake) { return; }

	// PWM 0. Later with motor APIs

	link_state = LINK_OFFLINE;

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_SET);
}

/**
 * @brief Exit the failsafe state after a valid ESP32 handshake.
 */
void Failsafe_Exit(void) {
	link_state = LINK_ONLINE;

	if (boot_handshake) {
		boot_handshake = false;

		HAL_GPIO_WritePin(GPIOD, BLUE_LED, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_RESET);
	}

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_SET);

}

/* -------------------------------------------------------------- */


/* ------------------ UART Transmission ------------------ */

static uint32_t last_req_sent = 0;

/**
 * @brief Periodically transmit STM32 handshake requests.
 */
void Process_UART_Tx(void) {
	if (link_state != LINK_ONLINE) {
		link_state = LINK_HANDSHAKING;

		if (HAL_GetTick() - last_req_sent >= RDT_STM_REQUEST_PERIOD_MS) {
			uint8_t request = RDT_STM_REQ;

			RDT_Result_t result =
				RDT_SendFrame(
					RDT_HANDSHAKE_TYPE,
					&request,
					RDT_HANDSHAKE_LENGTH
				);

			if (result != RDT_RESULT_OK) {
				return;
			}

			last_req_sent = HAL_GetTick();
		}
	}
}

/* -------------------------------------------------------------- */
