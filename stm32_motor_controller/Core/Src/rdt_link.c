#include "rdt_link.h"
#include "stm32f4xx_hal_hardware_config.h"

RDT_Parser_t rdt_parser;

RDT_Result_t RDT_SendFrame(const uint8_t type, const uint8_t *payload, uint8_t payload_length) {
	if (payload == ((void*) 0) && payload_length > 0U) {
		return RDT_RESULT_INVALID_FRAME;
	}

	RDT_Frame_t tx_frame = { 0 };

	tx_frame.type = type;
	for (uint8_t i = 0; i < payload_length; ++i) {
		tx_frame.payload[i] = payload[i];
	}
	tx_frame.length = payload_length;

	uint8_t tx_buffer[520];
	uint16_t tx_length = 0U;

	const RDT_Result_t result =
		RDT_Frame_Encode(
			&tx_frame,
			tx_buffer,
			sizeof(tx_buffer),
			&tx_length
	    );

	if (result != RDT_RESULT_OK) {
		return result;
	}

	if (HAL_UART_Transmit_IT(&huart1, tx_buffer, tx_length) != HAL_OK) {
		return RDT_RESULT_INVALID_FRAME;
	}

	return RDT_RESULT_OK;
}

void RDT_Link_Init(void) {
	RDT_Parser_Init(&rdt_parser);
}
