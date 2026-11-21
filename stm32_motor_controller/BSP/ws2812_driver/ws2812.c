/**
 * @file    ws2812.c
 * @brief   WS2812 Driver source file.
 *
 * @details This file implements all the necessary APIs to control WS2812 LEDs.
 */

#include "ws2812.h"
#include "stm32f4xx_hal_hardware_config.h"

/* ------------------ General Macros Definitions ------------------ */

#define WS_NUMBER_OF_LEDS        19
#define WS_BITS_PER_LED          24
#define WS_RESET_SLOTS           50   // 50 * 1.25us = 62.5us LOW
#define WS_DATA_LEN              (WS_NUMBER_OF_LEDS * WS_BITS_PER_LED)
#define WS_BUFFER_CAPACITY       (WS_DATA_LEN + WS_RESET_SLOTS)
#define WS_STATUS_LEDS           12
#define WS_HEADLIGHT_LEDS        7
#define WS_HEADLIGHT_BRIGHTNESS  150U

#define WS_BIT_1                 59   // ~0.7us HIGH
#define WS_BIT_0                 29   // ~0.35us HIGH

/* ---------------------------------------------------------------- */


/* ------------------ Static Helper Functions and Variables ------------------ */

static uint16_t ws_dma_buffer[WS_BUFFER_CAPACITY] = { 0 };

static volatile WS_State_t ws_dma_state = WS_READY;

static uint8_t ws_status_brightness = 0;

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

/* ---------------------------------------------------------------- */
