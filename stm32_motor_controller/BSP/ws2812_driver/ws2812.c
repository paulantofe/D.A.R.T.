/**
 * @file    ws2812.c
 * @brief   WS2812 Driver source file.
 *
 * @details This file implements all the necessary APIs to control WS2812 LEDs.
 */

#include "ws2812.h"
#include "stm32f4xx_hal_hardware_config.h"

/* ------------------ Static Helper Functions and Variables ------------------ */

static uint16_t ws_dma_buffer[WS_BUFFER_CAPACITY] = { 0 };

static volatile WS_State_t ws_dma_state = WS_READY;

static uint8_t ws_status_brightness = 0;

static volatile bool update_status_flag = false;
static volatile uint8_t pending_r = 0,
		                pending_g = 0,
					    pending_b = 0;

static void WS_Reset_DmaBuffer(void) {
	for (uint16_t i = 0; i < WS_DATA_LEN; ++i) {
		ws_dma_buffer[i] = WS_BIT_0;
	}

	for (uint16_t i = WS_DATA_LEN; i < WS_BUFFER_CAPACITY; ++i) {
		ws_dma_buffer[i] = 0;
	}
}

/* --------------------------------------------------------------------------- */


/* ------------------ WS2812 APIs Implementation ------------------ */

void WS_Init(void) {
	TIM4_Config();
	DMA1_Config();
	ws_status_brightness = 100U;
	ws_dma_state = WS_READY;

	WS_CloseLEDs();
}

void WS_OnDmaComplete(void) {
	HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_3);
	ws_dma_state = WS_READY;
}

void WS_SendPacket(void) {
	if (ws_dma_state == WS_BUSY) {
		return;
	}

	ws_dma_state = WS_BUSY;

	if (HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_3, (uint32_t *)ws_dma_buffer, WS_BUFFER_CAPACITY) != HAL_OK) {
		Error_Handler();
	}
}

bool WS_CloseLEDs(void) {
	if (ws_dma_state == WS_BUSY) { return false; }

	WS_Reset_DmaBuffer();
	WS_SendPacket();

	return true;
}

WS_State_t WS_GetState(void) {
	return ws_dma_state;
}

bool WS_SetPixel(uint8_t pixel_id, uint8_t r, uint8_t g, uint8_t b) {
	if (ws_dma_state == WS_BUSY || pixel_id >= WS_NUMBER_OF_LEDS) {
		return false;
	}

	uint8_t ws_brightness = (pixel_id < WS_STATUS_LEDS) ? ws_status_brightness : WS_HEADLIGHT_BRIGHTNESS;

	uint16_t scale = (uint16_t)ws_brightness + 1;
	r = (uint8_t)((r * scale) >> 8);
	g = (uint8_t)((g * scale) >> 8);
	b = (uint8_t)((b * scale) >> 8);

	uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
	uint16_t offset = pixel_id * WS_BITS_PER_LED;

	for (int8_t bit = 23; bit >= 0; --bit) {
		if (color & (1U << bit)) {
			ws_dma_buffer[offset++] = WS_BIT_1;
		} else {
			ws_dma_buffer[offset++] = WS_BIT_0;
		}
	}

	return true;
}

void WS_SetStatusBrightness(uint8_t brightness) {
	ws_status_brightness = brightness;
}

void WS_SetStatusColor(uint8_t r, uint8_t g, uint8_t b) {
	while (ws_dma_state == WS_BUSY);

	for (uint8_t i = 0; i < WS_STATUS_LEDS; ++i) {
		WS_SetPixel(i, r, g, b);
	}

	WS_SendPacket();
}

void WS_RequestStatusColor(uint8_t r, uint8_t g, uint8_t b) {
	pending_r = r;
	pending_g = g;
	pending_r = b;
	update_status_flag = true;
}

/* ---------------------------------------------------------------- */
