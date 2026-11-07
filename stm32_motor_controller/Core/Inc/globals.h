#ifndef INC_GLOBALS_H_
#define INC_GLOBALS_H_

#include <stdint.h>

#define RX_BUFFER_CAPACITY    30

extern uint8_t rx_byte;
extern uint8_t rx_buffer[RX_BUFFER_CAPACITY];
extern volatile uint8_t rx_overrun;
extern volatile uint8_t rx_error;

#endif
