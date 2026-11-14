#ifndef RDT_LINK_H
#define RDT_LINK_H 

#include <rdt_protocol.h>
#include <Arduino.h>

RDT_Result_t RDT_SendFrame(const uint8_t type, const uint8_t *payload, uint8_t payload_length);

void RDT_Link_Init(void);

void RDT_Link_CheckTimeout(uint32_t now);

void RDT_Handle_IncomingByte(const uint8_t received);

void RDT_Handle_Handshake(RDT_Frame_t *frame);

void RDT_Handle_Status(RDT_Frame_t *frame);

#endif
