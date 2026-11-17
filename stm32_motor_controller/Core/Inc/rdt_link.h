/**
 * @file     rdt_link.h
 * @brief    STM32-side RDT UART link API.
 */

#ifndef INC_RDT_LINK_H
#define INC_RDT_LINK_H

#include <stdint.h>
#include <rdt_protocol.h>

/* ------------------ RDT Link APIs ------------------ */

/**
 * @brief Encode and transmit one logical RDT frame.
 * @param[in] type Message type.
 * @param[in] payload Unescaped payload bytes.
 * @param[in] payload_length Number of payload bytes.
 * @return The frame encoding result.
 */
RDT_Result_t RDT_SendFrame(const uint8_t type, const uint8_t *payload, uint8_t payload_length);

/**
 * @brief Initialize the ESP32 RDT parser and link state.
 */
void RDT_Link_Init(void);

/**
 * @brief Submit one received UART byte to the RDT parser.
 * @param[in] received Received UART byte.
 */
void RDT_Handle_IncomingByte(const uint8_t received);

/**
 * @brief Reset the UART transmit buffer and release the transmit lock.
 */
void RDT_Reset_TxBuffer(void);

/**
 * @brief Apply the STM32 control-command watchdog.
 * @param[in] now Current HAL tick value.
 */
void RDT_Link_CheckTimeout(uint32_t now);

/* -------------------------------------------------- */

#endif
