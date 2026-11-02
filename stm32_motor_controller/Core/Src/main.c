#include "main.h"

int main(void) {
	HAL_Init();

	while (1) {

	}
}

void Error_Handler(void) {
	__disable_irq();

	while (1);
}
