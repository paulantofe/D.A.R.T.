#include <Arduino.h>
#include "app.h"
#include "globals.h"

/* ------------------ System Fault Handler ------------------ */

#define SEND_X_FREQ_MS    50U

void SystemFault_Enter(void) {
    system_state = System_State_t::SystemFault;

    Serial.println("[SYSTEM] Encountered System Fault. Please reboot the rover.");
    
    while (true) {
        Serial1.print('X');
        delay(SEND_X_FREQ_MS);
    }    
}

/* ---------------------------------------------------------- */