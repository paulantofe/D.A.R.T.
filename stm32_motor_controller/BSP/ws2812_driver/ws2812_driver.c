/**
 * @file    ws2812_driver.c
 * @brief   WS2812 driver implementation.
 * @details Implements WS2812 buffer management and STM32 HAL timer/DMA
 *          integration.
 */

#include "ws2812_driver.h"

/* ------------------ Static Helper Functions, Variables and Macros ------------------ */

#define WS_BYTES_PER_LED       3U

#define WS_MAX_LEDS            256U
#define WS_MAX_RESET_SLOTS     64U
#define WS_MAX_PIXEL_DATA_LEN  (WS_MAX_LEDS * WS_BYTES_PER_LED)
#define WS_MAX_PWM_DATA_LEN    ((WS_MAX_LEDS * WS_BITS_PER_LED) + WS_MAX_RESET_SLOTS)

static uint8_t pixel_data[WS_MAX_PIXEL_DATA_LEN]; /**< Internal LED color buffer */
static uint16_t pwm_data[WS_MAX_PWM_DATA_LEN];    /**< Internal timer/DMA buffer */

/* ---------------------------------------------------------------------------------- */


/* ------------------ WS2812 APIs Implementation ------------------ */

HAL_StatusTypeDef WS_Init(WS_Handle_t *hws) {
	if (hws == NULL) {
		return HAL_ERROR;
	}

	hws->is_initialized = false;
	hws->transfer_active = false;

	if (hws->config.htim_pwm == NULL || hws->config.led_count == 0U
			|| hws->config.led_count > WS_MAX_LEDS
			|| hws->config.reset_slots > WS_MAX_RESET_SLOTS
			|| hws->config.bit_0_duty > hws->config.htim_pwm->Init.Period
			|| hws->config.bit_1_duty > hws->config.htim_pwm->Init.Period) {
		return HAL_ERROR;
	}

	hws->pixel_data_len = (uint32_t) hws->config.led_count * WS_BYTES_PER_LED;

	hws->pwm_data_len = ((uint32_t) hws->config.led_count * WS_BITS_PER_LED)
			+ hws->config.reset_slots;

	hws->is_initialized = true;

	return HAL_OK;
}

/* ---------------------------------------------------------------- */
