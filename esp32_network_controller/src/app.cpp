/**
 * @file     app.cpp
 * @brief    ESP32 application runtime implementation.
 */

#include <Arduino.h>
#include "app.h"
#include "globals.h"
#include "camera.h"
#include "server.h"

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

void SystemState_Update(void) {
    if (system_state == System_State_t::SystemFault) {
        return;
    }

    if (link_state == Link_State_t::LINK_ONLINE) {
        if (system_state == System_State_t::Offline ||
            system_state == System_State_t::FailSafe) {
            system_state = System_State_t::Idle;
        }

        return;
    }

    if (link_state != Link_State_t::LINK_ONLINE &&
        (system_state == System_State_t::Idle ||
         system_state == System_State_t::Moving)) {
        system_state = System_State_t::FailSafe;
    }
}

String Get_SystemState_String(void) {
    return system_state == System_State_t::FailSafe    ? "failsafe" :
           system_state == System_State_t::Idle        ? "idle"     :
           system_state == System_State_t::Moving      ? "moving"   :
           system_state == System_State_t::Offline     ? "offline"  :
           system_state == System_State_t::SystemFault ? "systemfault" : "";        
}

/* ---------------------------------------------------------- */


/* ------------------ Boot Handler ------------------ */

#define UART_RX_PIN     14
#define UART_TX_PIN     15

void ESP_Boot(void) {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

    if (Camera_Init() != ESP_OK) {
        SystemFault_Enter();
    }

    if (Server_Init() != ESP_OK) {
        SystemFault_Enter();
    }

    Serial.println("[SYSTEM] ESP32 initialization successful. Waiting for STM32...");
}

/* -------------------------------------------------- */


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

String Get_LinkState_String(void) {
    return link_state == Link_State_t::LINK_OFFLINE     ? "offline" :
           link_state == Link_State_t::LINK_ONLINE      ? "online"  :
           link_state == Link_State_t::LINK_HANDSHAKING ? "handshaking" : "";
}

/* ------------------------------------------------------- */


/* ------------------ Serial1 Handler ------------------ */

void Serial1_Process(void) {
    while (Serial1.available() > 0) {
        const char received = Serial1.read();
        const uint32_t now = millis();

        if (received == 'R') {
            LinkHandshake_ProcessByte(received, now);
        } 
        else if (received == 'B') {
            // Battery data reception is not implemented yet.
        }
    }

    LinkHandshake_CheckTimeout(millis());
}

/* ----------------------------------------------------- */


/* ------------------ Driver Connection Handler ------------------ */

#define DRIVER_TIMEOUT_MS      2000U

volatile uint32_t last_status_request = 0;

void Check_DriverConnection(void) {
    if (has_active_client) {
        if (millis() - last_status_request > DRIVER_TIMEOUT_MS) {
            has_active_client = false;
            active_client_ip = { 0, 0, 0, 0 };

            Serial.println("[SYSTEM] Driver disconnected. Timeout Elapsed.");
        }
    }
}

/* --------------------------------------------------------------- */