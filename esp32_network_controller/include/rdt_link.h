/**
 * @file     rdt_link.h
 * @brief    ESP32-side RDT UART link API.
 */

#ifndef RDT_LINK_H
#define RDT_LINK_H

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
 * @brief Process link watchdogs and handshake/control timing.
 * @param[in] now Current millisecond timestamp.
 */
void RDT_Link_CheckTimeout(uint32_t now);

/**
 * @brief Submit one received UART byte to the RDT parser.
 * @param[in] received Received UART byte.
 */
void RDT_Handle_IncomingByte(const uint8_t received);

/**
 * @brief Process a decoded handshake frame.
 * @param[in] frame Decoded logical handshake frame.
 */
void RDT_Handle_Handshake(RDT_Frame_t *frame);

/**
 * @brief Process a decoded status frame.
 * @param[in] frame Decoded logical status frame.
 */
void RDT_Handle_Status(RDT_Frame_t *frame);

/* -------------------------------------------------- */

#endif
