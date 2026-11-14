/**
 * @file     app.cpp
 * @brief    ESP32 application runtime implementation.
 */

#include <Arduino.h>
#include "app.h"
#include "globals.h"
#include "camera.h"
#include "server.h"
#include "rdt_link.h"

/* ------------------ System Fault Handler ------------------ */

void SystemFault_Enter(void) {
    system_state = System_State_t::SystemFault;

    Serial.println("[SYSTEM] Encountered System Fault. Please reboot the rover.");
    
    const uint8_t response = RDT_CONTROL_SYSTEM_FAULT;

    while (true) {
        RDT_SendFrame(RDT_CONTROL_TYPE, &response, RDT_CONTROL_LENGTH);
        
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

    RDT_Link_Init();

    Serial.println("[SYSTEM] ESP32 initialization successful. Waiting for STM32...");
}

/* -------------------------------------------------- */


/* ------------------ UART Processing ------------------ */

String Get_LinkState_String(void) {
    return link_state == Link_State_t::LINK_OFFLINE     ? "offline" :
           link_state == Link_State_t::LINK_ONLINE      ? "online"  :
           link_state == Link_State_t::LINK_HANDSHAKING ? "handshaking" : "";
}

/**
 * @brief Read all pending Serial1 bytes and pass them to the RDT parser.
 */
static void Serial1_Receive(void) {
    while (Serial1.available() > 0) {
        uint8_t received = static_cast<uint8_t>(Serial1.read());

        RDT_Handle_IncomingByte(received);
    }
}

/** Timestamp of the most recently transmitted control frame. */
static uint32_t last_command_sent = 0;

/**
 * @brief Periodically transmit the current validated control command.
 */
static void Serial1_Send(void) {
    if (link_state == LINK_HANDSHAKING || link_state == LINK_OFFLINE) {
        return;
    }
    
    if (!RDT_Control_IsValid(current_command)) {
        return;
    }

    const uint32_t now = millis();

    if (now - last_command_sent < RDT_ESP_CONTROL_PERIOD_MS) {
        return;
    }

    if (RDT_SendFrame(
            RDT_CONTROL_TYPE,
            &current_command,
            RDT_CONTROL_LENGTH
        ) == RDT_RESULT_OK) {
        last_command_sent = now;
    }
}

/**
 * @brief Run the ESP32 Serial1 receive and transmit services.
 */
void Process_Serial1(void) {
    Serial1_Receive();
    Serial1_Send();
}

/* -------------------------------------------------- */


/* ------------------ Driver Connection Handler ------------------ */

volatile uint32_t last_status_request = 0;

void DriverConnection_CheckTimeout(void) {
    if (has_active_client) {
        if (millis() - last_status_request > RDT_BROWSER_TIMEOUT_MS) {
            has_active_client = false;
            active_client_ip = { 0, 0, 0, 0 };
            current_command = RDT_CONTROL_FAILSAFE;

            Serial.println("[SYSTEM] Driver disconnected. Timeout Elapsed.");
        }
    }
}

/* --------------------------------------------------------------- */