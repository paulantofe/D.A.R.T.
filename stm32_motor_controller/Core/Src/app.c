#include "app.h"
#include <stdbool.h>
#include "globals.h"
#include "stm32f4xx_hal_hardware_config.h"

void Failsafe_enter(void) {
	// PWM 0. Later with motor APIs

	link_state = LINK_OFFLINE;

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_SET);
}

static bool boot_handshake = true;

void Failsafe_exit(void) {
	link_state = LINK_ONLINE;

	if (boot_handshake) {
		boot_handshake = false;

		HAL_GPIO_WritePin(GPIOD, BLUE_LED, GPIO_PIN_RESET);
	} else {
		HAL_GPIO_WritePin(GPIOD, ORANGE_LED, GPIO_PIN_RESET);
	}

	HAL_GPIO_WritePin(GPIOD, GREEN_LED, GPIO_PIN_SET);

}
