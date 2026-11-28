/**
 * @file    lighting_service.c
 * @brief   Rover lighting service implementation.
 * @details Coordinates the onboard status LEDs and the WS2812 LED strip
 *          used for system-state and headlight indications.
 */

#include "lighting_service.h"
#include "ws2812_driver.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "board_peripherals.h"
#include <stdbool.h>

/* ------------------ Static Service Instances ------------------ */

/* Lighting colors used by the status and headlight patterns. */
static const WS_Color_t headlight_color = { 255U, 250U, 244U };
static const WS_Color_t boot_color = { 0U, 0U, 255U };
static const WS_Color_t sysfault_color = { 255U, 0U, 0U };
static const WS_Color_t failsafe_color = { 255U, 165U, 0U };
static const WS_Color_t online_color = { 0U, 255U, 0U };

static WS_Handle_t hws;             /**< Internal WS2812 driver instance. */
static bool is_initialized = false; /**< True after successful initialization. */

/* ---------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef LS_Init(void) {
	is_initialized = false;
	LED_Config();
	HAL_GPIO_WritePin(
		GPIOD,
		GREEN_LED | ORANGE_LED | RED_LED | BLUE_LED,
		GPIO_PIN_RESET
	);

	if (TIM4_Config() != HAL_OK || DMA1_Config() != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	hws.config.bit_0_duty = 29;
	hws.config.bit_1_duty = 59;
	hws.config.brightness = LS_LED_BRIGHTNESS;
	hws.config.color_order = WS_COLOR_ORDER_GRB;
	hws.config.htim_pwm = &htim4;
	hws.config.led_count = 19;
	hws.config.pwm_channel = TIM_CHANNEL_3;
	hws.config.reset_slots = 50;

	if (WS_Init(&hws) != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	is_initialized = true;

	if (WS_TransmitBlocking(&hws) != HAL_OK) {
		is_initialized = false;
		SS_RequestState(SS_State_Systemfault);
		return HAL_ERROR;
	}

	return HAL_OK;
}


HAL_StatusTypeDef LS_HeadlightControl(uint8_t OnorOff) {
	if (!is_initialized) {
		return HAL_ERROR;
	}

	if (OnorOff == LS_HEADLIGHT_ON) {
		HAL_StatusTypeDef status = WS_FillRange(&hws, LS_HEADLIGHT_LED_START,
				LS_HEADLIGHT_LED_COUNT, headlight_color);

		if (status != HAL_OK) {
			return status;
		}
	} else if (OnorOff == LS_HEADLIGHT_OFF){
		HAL_StatusTypeDef status = WS_ClearRange(&hws, LS_HEADLIGHT_LED_START,
		        LS_HEADLIGHT_LED_COUNT);

		if (status != HAL_OK) {
			return status;
		}
	} else {
		return HAL_ERROR;
	}

	return WS_RequestTransmit(&hws);
}


HAL_StatusTypeDef LS_SetStateColor(SS_State_t state) {
	if (!is_initialized) {
		return HAL_ERROR;
	}

	switch (state) {
	case SS_State_Systemfault: {
		HAL_GPIO_WritePin(
		GPIOD,
		GREEN_LED | ORANGE_LED | BLUE_LED, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, RED_LED, GPIO_PIN_SET);

		HAL_StatusTypeDef status = WS_FillRange(&hws, LS_STATUS_LED_START,
		LS_STATUS_LED_COUNT, sysfault_color);

		if (status != HAL_OK) {
			return status;
		}

		return WS_TransmitBlocking(&hws);
	}

	case SS_State_Boot: {
		HAL_GPIO_WritePin(
		GPIOD,
		GREEN_LED | ORANGE_LED | RED_LED, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, BLUE_LED, GPIO_PIN_SET);

		HAL_StatusTypeDef status = WS_FillRange(&hws, LS_STATUS_LED_START,
		LS_STATUS_LED_COUNT, boot_color);

		if (status != HAL_OK) {
			return status;
		}

		return WS_RequestTransmit(&hws);
	}

	case SS_State_Failsafe: {
		HAL_GPIO_WritePin(
		GPIOD,
		GREEN_LED | RED_LED | BLUE_LED, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_SET);

		HAL_StatusTypeDef status = WS_FillRange(&hws, LS_STATUS_LED_START,
		LS_STATUS_LED_COUNT, failsafe_color);

		if (status != HAL_OK) {
			return status;
		}

		return WS_RequestTransmit(&hws);
	}

	case SS_State_Idle:
	case SS_State_Moving: {
		HAL_GPIO_WritePin(
		GPIOD,
		ORANGE_LED | RED_LED | BLUE_LED, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_SET);

		HAL_StatusTypeDef status = WS_FillRange(&hws, LS_STATUS_LED_START,
		LS_STATUS_LED_COUNT, online_color);

		if (status != HAL_OK) {
			return status;
		}

		return WS_RequestTransmit(&hws);
	}

	default:
		return HAL_ERROR;
	}
}


HAL_StatusTypeDef LS_Process(void) {
	if (!is_initialized) {
		return HAL_ERROR;
	}

	return WS_Process(&hws);
}

void LS_OnDmaComplete(void) {
	if (!is_initialized) {
		return;
	}

	WS_OnDmaComplete(&hws);
}

/* ---------------------------------------------------------------- */
