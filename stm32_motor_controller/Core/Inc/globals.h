#ifndef INC_GLOBALS_H_
#define INC_GLOBALS_H_

#include <stdint.h>
#include <stdbool.h>
#include "app.h"

extern volatile System_State_t system_state;
extern volatile Link_State_t link_state;

extern volatile uint8_t current_command;

extern volatile uint8_t received_byte;

extern volatile bool boot_handshake;

#endif
