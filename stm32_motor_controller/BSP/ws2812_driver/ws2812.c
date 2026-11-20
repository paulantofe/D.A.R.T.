/**
 * @file    ws2812.c
 * @brief   WS2812 Driver source file.
 *
 * @details This file implements all the necessary APIs to control WS2812 LEDs.
 */

#include "ws2812.h"
#include "stm32f4xx_hal_hardware_config.h"

/* ------------------ General Macros Definitions ------------------ */

#define WS_NUMBER_OF_LEDS     19
#define WS_BITS_PER_LED       24
#define WS_RESET_SLOTS        50   // 50 * 1.25us = 62.5us LOW
#define WS_DATA_LEN           (WS_NUMBER_OF_LEDS * WS_BITS_PER_LED)
#define WS_BUFFER_CAPACITY    (WS_DATA_LEN + WS_RESET_SLOTS)

#define WS_BIT_1              59   // ~0.8us HIGH
#define WS_BIT_0              29   // ~0.4us HIGH

/* ---------------------------------------------------------------- */


/* ------------------ Static Helper Functions and Variables ------------------ */

static uint16_t ws_dma_buffer[WS_BUFFER_CAPACITY] = { 0 };

static WS_State_t ws_dma_state = WS_READY;

static void WS_Reset_DmaBuffer(void) {
	for (uint16_t i = 0; i < WS_DATA_LEN; ++i) {
		ws_dma_buffer[i] = WS_BIT_0;
	}

	for (uint16_t i = WS_DATA_LEN; i < WS_BUFFER_CAPACITY; ++i) {
		ws_dma_buffer[i] = 0;
	}
}

static void WS_SendPacket(void) {
	if (ws_dma_state == WS_BUSY) {
		return;
	}

	ws_dma_state = WS_BUSY;

	if (HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_3, (uint32_t *)ws_dma_buffer, WS_BUFFER_CAPACITY) != HAL_OK) {
		Error_Handler();
	}
}

/* --------------------------------------------------------------------------- */


/* ------------------ WS2812 APIs Implementation ------------------ */

void WS_Init(void) {
	TIM4_Config();
	DMA1_Config();

	WS_CloseLEDs();
}

void WS_OnDmaComplete(void) {
	HAL_TIM_PWM_Stop_DMA(&htim4, TIM_CHANNEL_3);
	ws_dma_state = WS_READY;
}

bool WS_CloseLEDs(void) {
	if (ws_dma_state == WS_BUSY) { return false; }

	WS_Reset_DmaBuffer();
	WS_SendPacket();

	return true;
}

/* ---------------------------------------------------------------- */
