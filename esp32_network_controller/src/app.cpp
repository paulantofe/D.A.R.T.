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


/* ------------------ Handshake Handler ------------------ */

#define REQUIRED_R_COUNT       3U
#define R_INTERVAL_TIMEOUT_MS  100U

static uint8_t r_count;
static uint32_t last_r_time;
static bool reset_handshake = true;

void LinkHandshake_ProcessByte(const char received, const uint32_t now) {
    if (received != 'R') { return; }
    
    if (reset_handshake) {
        reset_handshake = false;

        link_state = Link_State_t::LINK_HANDSHAKING;
        r_count = 1;
        last_r_time = now;
        
        return;
    }

    if (now - last_r_time > R_INTERVAL_TIMEOUT_MS) {
        r_count = 1;
        link_state = LINK_HANDSHAKING;
        last_r_time = now;
        return;
    }

    ++r_count;

    last_r_time = now;

    if (r_count >= REQUIRED_R_COUNT) {
        link_state = Link_State_t::LINK_ONLINE;
        Serial1.write('H');
    }
}

void LinkHandshake_CheckTimeout(const uint32_t now) {
    if (link_state == LINK_ONLINE && now - last_r_time > R_INTERVAL_TIMEOUT_MS) {
        link_state = LINK_OFFLINE;
        reset_handshake = true;
        r_count = 0;
    }
}

/* ------------------------------------------------------- */


/* ------------------ Serial1 Handler ------------------ */

void Serial1_Process() {
    while (Serial1.available() > 0) {
        const char received = Serial1.read();
        const uint32_t now = millis();

        if (received == 'R') {
            LinkHandshake_ProcessByte(received, now);
        } 
        else if (received == 'B') {
            // battery reception
            // for later
        }
    }

    LinkHandshake_CheckTimeout(millis());
}

/* ------------------------------------------------------- */