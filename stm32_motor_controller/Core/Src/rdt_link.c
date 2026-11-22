/**
 * @file     rdt_link.c
 * @brief    STM32-side RDT framing, UART transport, and link watchdog logic.
 */

#include "rdt_link.h"
#include "globals.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_hardware_config.h"
#include <stdbool.h>
#include <stdint.h>

/** Parser state for the incoming STM32 UART stream. */
RDT_Parser_t rdt_parser;

/* ------------------ Transmit Buffer ------------------ */

/** Shared interrupt-driven UART transmit buffer. */
uint8_t tx_buffer[520];
/** Number of encoded bytes currently in tx_buffer. */
uint16_t tx_length = 0U;
/** Indicates that an interrupt-driven UART transmission is active. */
volatile uint8_t tx_busy = false;

void RDT_Reset_TxBuffer(void) {
	for (uint16_t i = 0; i < 520; ++i) {
		tx_buffer[i] = 0;
	}

	tx_length = 0;

	tx_busy = false;
}

/* ------------------------------------------------------------ */


/* ------------------ Link Watchdog State ------------------ */

static volatile uint32_t last_rec_command = 0;

/* ------------------------------------------------------------ */


/* ------------------ Static Helper Functions ------------------ */

/**
 * @brief Process a valid ESP32 handshake response.
 * @param[in] frame Parsed RDT handshake frame.
 */
static void RDT_Handle_Handshake(RDT_Frame_t *frame) {
	if (frame == ((void*) 0) || frame->type != RDT_HANDSHAKE_TYPE ||
			frame->payload[0] != RDT_ESP_OK) {
		return;
	}

	last_rec_command = HAL_GetTick();
	Failsafe_Exit();
}

static uint8_t last_headlight_cmd = 0;
static uint8_t last_motor_cmd = 0;

/**
 * @brief Process a valid control command.
 * @param[in] frame Parsed RDT control frame.
 */
static void RDT_Handle_Command(RDT_Frame_t *frame) {
	if (frame == ((void*) 0) || frame->type != RDT_CONTROL_TYPE ||
			frame->length != RDT_CONTROL_LENGTH) {
		return;
	}

	uint8_t motor_cmd = 0;
    uint8_t headlight_cmd = 0;

    if (RDT_Command_Decode(
    		frame->payload,
		frame->length,
		&motor_cmd,
		&headlight_cmd) != RDT_RESULT_OK) {
        return;
    }

    if (!RDT_Control_Motor_IsValid(motor_cmd) || !RDT_Control_Headlight_IsValid(headlight_cmd)) {
    	    return;
    }

	last_rec_command = HAL_GetTick();

	if (last_motor_cmd != motor_cmd) {
		last_motor_cmd = motor_cmd;
		current_motor_command = motor_cmd;
	}

	if (last_headlight_cmd != headlight_cmd) {
		last_headlight_cmd = headlight_cmd;
		WS_RequestHeadlightControl(headlight_cmd);
	}
}

/* ------------------------------------------------------------- */


/* ------------------ RDT Frame Transmission ------------------ */

RDT_Result_t RDT_SendFrame(const uint8_t type, const uint8_t *payload, uint8_t payload_length) {
	if (payload == ((void*) 0) && payload_length > 0U) {
		return RDT_RESULT_INVALID_FRAME;
	}

	if (tx_busy) {
		return RDT_RESULT_BUSY;
	}
	else {
		tx_busy = true;
	}

	RDT_Frame_t tx_frame = { 0 };

	tx_frame.type = type;
	for (uint8_t i = 0; i < payload_length; ++i) {
		tx_frame.payload[i] = payload[i];
	}
	tx_frame.length = payload_length;

	const RDT_Result_t result =
		RDT_Frame_Encode(
			&tx_frame,
			tx_buffer,
			sizeof(tx_buffer),
			&tx_length
	    );

	if (result != RDT_RESULT_OK) {
		RDT_Reset_TxBuffer();
		return result;
	}

	if (HAL_UART_Transmit_IT(&huart1, tx_buffer, tx_length) != HAL_OK) {
		RDT_Reset_TxBuffer();
		return RDT_RESULT_INVALID_FRAME;
	}

	return RDT_RESULT_OK;
}

/* ------------------------------------------------------------- */


/* ------------------ RDT Frame Reception ------------------ */

void RDT_Link_Init(void) {
	RDT_Parser_Init(&rdt_parser);
}

void RDT_Handle_IncomingByte(const uint8_t received) {
	RDT_Frame_t frame;

	const RDT_Result_t result =
		RDT_Parser_PushByte(&rdt_parser, received, &frame);

	if (result != RDT_RESULT_FRAME_READY) {
		return;
	}

	switch (frame.type) {
	    case RDT_HANDSHAKE_TYPE:
	    	RDT_Handle_Handshake(&frame);
	    	break;

	    case RDT_STATUS_TYPE:
	    	/* N/A for STM32 */
	    	break;

	    case RDT_CONTROL_TYPE:
	    	RDT_Handle_Command(&frame);
	    	break;

	    default:
	    	break;
	}
}

/* ------------------------------------------------------------- */


/* ------------------ RDT Link Watchdog ------------------ */

void RDT_Link_CheckTimeout(uint32_t now) {
	if (boot_handshake) {
		return;
	}

	if (now - last_rec_command >= RDT_STM_CONTROL_TIMEOUT_MS) {
		Failsafe_Enter();
		return;
	}
}

/* ------------------------------------------------------------- */
