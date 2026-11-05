/**
 * @file     main.cpp
 * @brief    Main project source file.
 * 
 * @details  This file initializes server and camera module. If any of these two fails, it triggers System_Fault state.
 *           It also manages the Network Failsafe (Triggers Failsafe state on browser timeout or connection lost) and
 *           hardware Failsafe (sends 'H' signal every 200ms to test uart connection)
 */

#include <Arduino.h>
#include "globals.h"
#include "server.h"
#include "camera.h"

/* -------------- Hardware Defines -------------- */

#define UART_TX_PIN    14
#define UART_RX_PIN    15

/* ---------------------------------------------- */


/* -------------- Global Variables Initialization -------------- */

String rover_battery = "N/A";
volatile uint32_t last_command_time = 0;
volatile bool has_active_client = false;

static uint32_t last_uart_heartbeat = 0;

constexpr uint32_t HANDSHAKE_TIMEOUT_MS = 5000;

/* ------------------------------------------------------------- */


/* -------------- Functions -------------- */

/**
 * @brief Handle errors by freezing the board
 */
void Error_Handler(void) {
    while (true) { delay(100); }
}

/**
 * @brief    Boot the ESP32-CAM Board
 * @details  Initialize Camera and Server module and trigger System_Fault state upon fail
 */
void ESP_Boot(void) {
    Serial.begin(115200);

    Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    Serial1.setTimeout(10);
    
    if (Camera_Init() != ESP_OK) {
        Serial.println("[CAMERA] Initialization failed. Triggering System_Fault.");
        Serial1.print('X');
        Serial1.flush();
        Error_Handler();
    }

    if (Server_Init() != ESP_OK) {
        Serial.println("[SERVER] Initialization failed. Triggering System_Fault.");
        Serial1.print('X');
        Serial1.flush();
        Error_Handler();
    }

    const uint32_t handshake_started = millis();
    bool handshake_ok = false;

    Serial.println("[SYSTEM] Waiting STM32 connection...");
    while (!handshake_ok && millis() - handshake_started < HANDSHAKE_TIMEOUT_MS) {
        if (Serial1.available() && Serial1.read() == 'R') {
            Serial1.write('K');
            handshake_ok = true;
        }

        if (!handshake_ok) { delay(1); }
    }

    if (!handshake_ok) {
        Serial.println("[SYSTEM] STM32 handshake timed out. Entering System Fault");
        Error_Handler();
    }

    Serial.println("[SYSTEM] System booted successfully. Ready to use.");
}

/* --------------------------------------- */


/* -------------- Tasks -------------- */

/**
 * @brief  Monitor WiFi client status and triggers 'F' Failsafe on timeout
 */
void Task_Network_Failsafe(uint32_t current_time) {
    if (current_time - last_command_time > 2500) {
        if (has_active_client) {
            // Let other browser become driver and trigger Failsafe
            has_active_client = false;
            Serial.println("[SERVER] Driver disconnected (Timeout elapsed)");
            Serial1.print('F');
        }
    }   
}

/**
 * @brief  Send 'H' Heartbeat to stm32 every 200ms to maintain UART link
 */
void Task_Hardware_Failsafe(uint32_t current_time) {
    if (current_time - last_uart_heartbeat > 200) {
        last_uart_heartbeat = current_time;
        Serial1.print('H');
    }
}

/**
 * @brief  Asynchronously read incoming UART messages from stm32
 */
void Task_Process_UART_RX(void) {
    if (Serial1.available()) {
        String msg = Serial1.readStringUntil('\n');
        msg.trim();

        if (msg.startsWith("B")) { rover_battery = msg.substring(1); }
    }
}

/* ----------------------------------- */


void setup() {
    ESP_Boot();
}

void loop() {
    uint32_t current_time = millis();

    Task_Network_Failsafe(current_time);
    Task_Hardware_Failsafe(current_time);
    Task_Process_UART_RX();

    delay(50);
}