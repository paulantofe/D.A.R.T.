/**
 * @file     app.c
 * @brief    STM32 application state and UART transmission logic.
 */

#include "app.h"
#include "globals.h"
#include "rdt_link.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "rmc_motor_controller.h"
#include "ws2812_driver/ws2812.h"
#include <stdbool.h>
#include <stdint.h>
#include <rdt_protocol.h>

/* ------------------ Shared Application State ------------------ */

/** True until the first valid ESP32 handshake response is received. */
volatile bool boot_handshake = true;

/* -------------------------------------------------------------- */


/* ------------------ Boot and Error Handler ------------------ */

void STM_Boot(void) {
	HAL_Init();

	SystemClock_Config();

	WS_Init();

	LED_Config();

	UART_Config();

	TIM6_Config();

	RMC_Motor_Init();

	RDT_Link_Init();

	WS_SetStatusColor(0, 0, 255);
	HAL_GPIO_WritePin(GPIOD, GREEN_LED | ORANGE_LED, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, BLUE_LED, GPIO_PIN_SET);

	HAL_UART_Receive_IT(&huart1, &received_byte, 1);
}

void Error_Handler(void) {
	__disable_irq();

	HAL_GPIO_WritePin(GPIOD, GREEN_LED | ORANGE_LED | BLUE_LED, GPIO_PIN_RESET);

	bool toggle = false;

	while (1) {
		toggle = !toggle;
		HAL_GPIO_TogglePin(GPIOD, RED_LED);

		if (toggle) {
			WS_ForceStatusColor(255, 0, 0);
		} else {
			WS_ForceStatusColor(0, 0, 0);
		}

		for (volatile uint32_t i = 0; i < 20000000; i++) { __NOP(); }
	}
}

/* ------------------------------------------------------------ */


/* ------------------ Failsafe State APIs ------------------ */

void Failsafe_Enter(void) {
	if (boot_handshake) { return; }

	if (link_state == LINK_OFFLINE) { return; }

	RMC_Motor_Stop();

	link_state = LINK_OFFLINE;

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_SET);

	WS_RequestStatusColor(255, 65, 0);
}


void Failsafe_Exit(void) {
	if (link_state == LINK_ONLINE && !boot_handshake) { return; }

	link_state = LINK_ONLINE;

	if (boot_handshake) {
		boot_handshake = false;

		HAL_GPIO_WritePin(GPIOD, BLUE_LED, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_RESET);
	}

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_SET);
    WS_RequestStatusColor(0, 255, 0);
}

/* --------------------------------------------------------- */


/* ------------------ UART Transmission ------------------ */

static uint32_t last_req_sent = 0;
static uint32_t last_status_sent = 0;

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

		return;
	}

	if (HAL_GetTick() - last_status_sent >= RDT_STM_STATUS_PERIOD_MS) {
		float current_speed = RMC_Motor_GetSpeed();
		float current_battery = 0.0f; // later will implement a battery getter

		uint8_t tx_payload[RDT_STATUS_LENGTH] = "";
		RDT_Result_t result =
			RDT_Status_Encode(
				current_battery,
			    current_speed,
			    tx_payload,
			    RDT_STATUS_LENGTH
		    );

		if (result != RDT_RESULT_OK) {
			return;
		}

		RDT_SendFrame(RDT_STATUS_TYPE, tx_payload, RDT_STATUS_LENGTH);

		last_status_sent = HAL_GetTick();
	}
}

/* ------------------------------------------------------- */
