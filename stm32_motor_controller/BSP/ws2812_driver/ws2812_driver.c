/**
 * @file    ws2812_driver.c
 * @brief   WS2812 driver implementation.
 * @details Implements WS2812 buffer management and STM32 HAL timer/DMA
 *          integration.
 */

#include "ws2812_driver.h"
#include "stm32f4xx_hal.h"
#include "board_peripherals.h"

/* ------------------ Static Helper Functions, Variables and Macros ------------------ */

#define WS_MAX_LEDS            256U
#define WS_MAX_RESET_SLOTS     64U
#define WS_MAX_PIXEL_DATA_LEN  (WS_MAX_LEDS * WS_BYTES_PER_LED)
#define WS_MAX_PWM_DATA_LEN    ((WS_MAX_LEDS * WS_BITS_PER_LED) + WS_MAX_RESET_SLOTS)
#define WS_TRANSMIT_TIMEOUT_MS 100U

static WS_Color_t pixel_data[WS_MAX_LEDS];     /**< Internal LED color buffer */
static uint16_t pwm_data[WS_MAX_PWM_DATA_LEN]; /**< Internal timer/DMA buffer */
static DMA_HandleTypeDef *WS_GetDmaHandle(const WS_Config_t *config) {
	if (config == NULL || config->htim_pwm == NULL) {
		return NULL;
	}

	switch (config->pwm_channel) {
	case TIM_CHANNEL_1:
		return config->htim_pwm->hdma[TIM_DMA_ID_CC1];
	case TIM_CHANNEL_2:
		return config->htim_pwm->hdma[TIM_DMA_ID_CC2];
	case TIM_CHANNEL_3:
		return config->htim_pwm->hdma[TIM_DMA_ID_CC3];
	case TIM_CHANNEL_4:
		return config->htim_pwm->hdma[TIM_DMA_ID_CC4];
	default:
		return NULL;
	}
}

static uint8_t WS_ApplyBrightness(uint8_t value, uint8_t brightness) {
	return (uint8_t)(((uint16_t)value * brightness + 127U) / 255U);
}

static void WS_EncodePixelData(const WS_Handle_t *hws) {
	uint32_t pwm_index = 0U;

	for (uint16_t led = 0U; led < hws->config.led_count; ++led) {
		const WS_Color_t *color = &pixel_data[led];
		uint8_t channels[WS_BYTES_PER_LED];

		switch (hws->config.color_order) {
		case WS_COLOR_ORDER_RGB:
			channels[0] = color->red;
			channels[1] = color->green;
			channels[2] = color->blue;
			break;
		case WS_COLOR_ORDER_GRB:
			channels[0] = color->green;
			channels[1] = color->red;
			channels[2] = color->blue;
			break;
		case WS_COLOR_ORDER_BGR:
			channels[0] = color->blue;
			channels[1] = color->green;
			channels[2] = color->red;
			break;
		default:
			channels[0] = 0U;
			channels[1] = 0U;
			channels[2] = 0U;
			break;
		}

		for (uint32_t channel = 0U; channel < WS_BYTES_PER_LED; ++channel) {
			uint8_t value =
				WS_ApplyBrightness(channels[channel], hws->config.brightness);

			for (uint8_t bit = 0U; bit < 8U; ++bit) {
				pwm_data[pwm_index++] =
					(value & (uint8_t)(1U << (7U - bit))) ?
						hws->config.bit_1_duty :
						hws->config.bit_0_duty;
			}
		}
	}

	for (uint16_t slot = 0U; slot < hws->config.reset_slots; ++slot) {
		pwm_data[pwm_index++] = 0U;
	}
}

/* ---------------------------------------------------------------------------------- */


/* ------------------ WS2812 APIs Implementation ------------------ */

HAL_StatusTypeDef WS_Init(WS_Handle_t *hws) {
	if (hws == NULL) {
		return HAL_ERROR;
	}

	hws->is_initialized = false;
	hws->transfer_active = false;
	hws->request_pending = false;

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

	hws->request_pending = false;

	hws->bypass_callback = false;

	hws->is_initialized = true;

	if (WS_ClearRange(hws, 0, hws->config.led_count) != HAL_OK) {
		hws->is_initialized = false;
		return HAL_ERROR;
	}

	hws->is_initialized = true;

	return HAL_OK;
}

HAL_StatusTypeDef WS_FillRange(WS_Handle_t *hws, uint16_t start_index,
		uint16_t count, WS_Color_t color) {
	if (hws == NULL || !hws->is_initialized) {
	    return HAL_ERROR;
	}

	if (hws->transfer_active) {
	    return HAL_BUSY;
	}

	if (count == 0U ||
	    start_index >= hws->config.led_count ||
	    count > (hws->config.led_count - start_index)) {
	    return HAL_ERROR;
	}

	for (uint16_t i = 0U; i < count; ++i) {
	    pixel_data[start_index + i] = color;
	}

	return HAL_OK;
}

HAL_StatusTypeDef WS_ClearRange(WS_Handle_t *hws, uint16_t start_index,
		uint16_t count) {
	if (hws == NULL || !hws->is_initialized) {
		return HAL_ERROR;
	}

	if (hws->transfer_active) {
		return HAL_BUSY;
	}

	if (count == 0U || start_index >= hws->config.led_count
			|| count > (hws->config.led_count - start_index)) {
		return HAL_ERROR;
	}

	WS_Color_t black = { 0, 0, 0 };
	return WS_FillRange(hws, start_index, count, black);
}

HAL_StatusTypeDef WS_TransmitBlocking(WS_Handle_t *hws) {
	if (hws == NULL || !hws->is_initialized) {
		return HAL_ERROR;
	}

	if (hws->transfer_active) {
		return HAL_BUSY;
	}

	DMA_HandleTypeDef *hdma = WS_GetDmaHandle(&hws->config);
	if (hdma == NULL || hws->pwm_data_len > UINT16_MAX) {
		return HAL_ERROR;
	}

	hws->bypass_callback = true;
	hws->transfer_active = true;
	WS_EncodePixelData(hws);
	__HAL_TIM_SET_COUNTER(hws->config.htim_pwm, 0U);

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(
			hws->config.htim_pwm,
			hws->config.pwm_channel,
			(const uint32_t *)pwm_data,
			(uint16_t)hws->pwm_data_len
		);

	if (status == HAL_OK) {
		__HAL_DMA_DISABLE_IT(
			hdma,
			DMA_IT_TC | DMA_IT_HT | DMA_IT_TE
		);
	}

	__set_PRIMASK(primask);

	if (status == HAL_OK) {
		status = HAL_DMA_PollForTransfer(
			hdma,
			HAL_DMA_FULL_TRANSFER,
			WS_TRANSMIT_TIMEOUT_MS
		);
	}

	(void)HAL_TIM_PWM_Stop_DMA(
		hws->config.htim_pwm,
		hws->config.pwm_channel
	);

	hws->transfer_active = false;
	hws->bypass_callback = false;

	return status;
}

HAL_StatusTypeDef WS_RequestTransmit(WS_Handle_t *hws) {
	if (hws == NULL || !hws->is_initialized) {
		return HAL_ERROR;
	}

	hws->request_pending = true;

	return HAL_OK;
}

HAL_StatusTypeDef WS_Process(WS_Handle_t *hws) {
	if (hws == NULL || !hws->is_initialized) {
		return HAL_ERROR;
	}

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	if (hws->transfer_active) {
		__set_PRIMASK(primask);
		return HAL_BUSY;
	}

	if (!hws->request_pending) {
		__set_PRIMASK(primask);
		return HAL_OK;
	}

	hws->transfer_active = true;
	hws->request_pending = false;

	__set_PRIMASK(primask);

	WS_EncodePixelData(hws);
	__HAL_TIM_SET_COUNTER(hws->config.htim_pwm, 0U);

	HAL_StatusTypeDef status = HAL_TIM_PWM_Start_DMA(
			hws->config.htim_pwm,
			hws->config.pwm_channel,
			(const uint32_t *)pwm_data,
			(uint16_t)hws->pwm_data_len
	);

	if (status != HAL_OK) {
		(void)HAL_TIM_PWM_Stop_DMA(
			hws->config.htim_pwm,
			hws->config.pwm_channel
		);
		hws->transfer_active = false;
		hws->request_pending = true;
	}

	return status;
}

void WS_OnDmaComplete(WS_Handle_t *hws) {
	if (hws == NULL || hws->bypass_callback) {
		return;
	}

	(void)HAL_TIM_PWM_Stop_DMA(
		hws->config.htim_pwm,
		hws->config.pwm_channel
	);

	hws->transfer_active = false;
}

/* ---------------------------------------------------------------- */
