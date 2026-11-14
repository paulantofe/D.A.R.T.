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

void SystemFault_Enter(void) {
    system_state = System_State_t::SystemFault;

    Serial.println("[SYSTEM] Encountered System Fault. Please reboot the rover.");
    
    while (true) {
        delay(50U);
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

static constexpr int8_t UART_RX_PIN = 14;
static constexpr int8_t UART_TX_PIN = 15;

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


String Get_LinkState_String(void) {
    return link_state == Link_State_t::LINK_OFFLINE     ? "offline" :
           link_state == Link_State_t::LINK_ONLINE      ? "online"  :
           link_state == Link_State_t::LINK_HANDSHAKING ? "handshaking" : "";
}

/* ------------------------------------------------------- */


/* ------------------ Driver Connection Handler ------------------ */

static constexpr uint32_t DRIVER_TIMEOUT_MS = 2000U;

volatile uint32_t last_status_request = 0;

void DriverConnection_CheckTimeout(void) {
    if (has_active_client) {
        if (millis() - last_status_request > DRIVER_TIMEOUT_MS) {
            has_active_client = false;
            active_client_ip = { 0, 0, 0, 0 };

            Serial.println("[SYSTEM] Driver disconnected. Timeout Elapsed.");
        }
    }
}

/* --------------------------------------------------------------- */