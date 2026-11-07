#include "app.h"
#include "globals.h"
#include "stm32f4xx_hal_hardware_config.h"

#define UART_HEARTBEAT_TIMEOUT_MS   200

uint8_t rx_byte = 0;
uint8_t rx_buffer[RX_BUFFER_CAPACITY] = { 0 };
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;
volatile uint8_t rx_overrun = 0;
volatile uint8_t rx_error = 0;

static uint32_t last_received_h = 0;
static uint8_t heartbeat_monitor_started = 0;
static uint8_t heartbeat_lost = 0;

void Task_Process_UART_RX(void) {
	if (rx_overrun != 0U || rx_error != 0U) {
		Failsafe_Handler();
		Error_Handler();
	}

	if (heartbeat_monitor_started == 0U) {
		last_received_h = HAL_GetTick();
		heartbeat_monitor_started = 1U;
	}

	while (rx_tail != rx_head) {
		const uint8_t byte = rx_buffer[rx_tail];
		rx_tail = (uint8_t)((rx_tail + 1U) % RX_BUFFER_CAPACITY);

		switch (byte) {
		case 'F':
			Failsafe_Handler();
			break;

		case 'H': {
			last_received_h = HAL_GetTick();
			heartbeat_lost = 0U;

			const uint8_t response = 'K';
			if (HAL_UART_Transmit(&huart1, &response, 1U, 10U) != HAL_OK) {
				Failsafe_Handler();
				Error_Handler();
			}
			break;
		}

		default:
			break;
		}
	}

	if (heartbeat_lost == 0U &&
		HAL_GetTick() - last_received_h >= UART_HEARTBEAT_TIMEOUT_MS) {
		heartbeat_lost = 1U;
		Failsafe_Handler();
	}
}
