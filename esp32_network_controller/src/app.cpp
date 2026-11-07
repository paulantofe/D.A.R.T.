/**
 * @file     app.cpp
 * @brief    Application boot, UART monitoring, and failsafe task implementations.
 */

#include <Arduino.h>
#include "app.h"
#include "globals.h"
#include "server.h"
#include "camera.h"

namespace {

constexpr uint8_t UART_TX_PIN = 14;
constexpr uint8_t UART_RX_PIN = 15;

constexpr uint32_t HANDSHAKE_TIMEOUT_MS = 5000;
constexpr uint32_t UART_HEARTBEAT_INTERVAL_MS = 50;
constexpr uint32_t UART_ACK_TIMEOUT_MS = 100;

uint32_t last_uart_heartbeat = 0;
uint32_t first_uart_heartbeat = 0;
uint32_t last_uart_acknowledgment = 0;
bool has_uart_acknowledgment = false;
uint8_t uart_acknowledgment_streak = 0;
bool has_sent_uart_heartbeat = false;
char uart_rx_line[32];
size_t uart_rx_line_length = 0;
bool uart_rx_discarding_line = false;

void Error_Handler(void) {
    Server_Stop();

    while (true) { delay(100); }
}

}

String rover_battery = "N/A";
volatile uint32_t last_command_time = 0;
volatile bool has_active_client = false;
volatile bool stm32_link_ok = false;
volatile bool stm32_link_status_known = false;

void ESP_Boot(void) {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

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

/**
 * @brief Monitor WiFi client status and trigger failsafe on command timeout.
 */
void Task_Network_Failsafe(uint32_t current_time) {
    if (current_time - last_command_time > 2500) {
        if (has_active_client) {
            // Let another browser become the driver and trigger failsafe.
            has_active_client = false;
            Serial.println("[SERVER] Driver disconnected (Timeout elapsed)");
            Serial1.print('F');
        }
    }
}

/**
 * @brief Send UART heartbeats and monitor STM32 acknowledgments.
 */
void Task_Hardware_Failsafe(uint32_t current_time) {
    if (current_time - last_uart_heartbeat >= UART_HEARTBEAT_INTERVAL_MS) {
        last_uart_heartbeat = current_time;
        if (!has_sent_uart_heartbeat) {
            first_uart_heartbeat = current_time;
            has_sent_uart_heartbeat = true;
        }
        Serial1.write('H');
    }

    const bool acknowledgment_recent = has_uart_acknowledgment &&
        current_time - last_uart_acknowledgment <= UART_ACK_TIMEOUT_MS;
    if (!acknowledgment_recent) {
        uart_acknowledgment_streak = 0;
    }

    if (has_sent_uart_heartbeat &&
        current_time - first_uart_heartbeat > UART_ACK_TIMEOUT_MS) {
        stm32_link_status_known = true;
    }

    const bool link_ok = acknowledgment_recent && uart_acknowledgment_streak >= 3;
    if (link_ok) {
        stm32_link_status_known = true;
    }

    if (link_ok != stm32_link_ok) {
        stm32_link_ok = link_ok;
        if (stm32_link_status_known) {
            Serial.println(link_ok
                ? "[SYSTEM] STM32 UART link restored."
                : "[SYSTEM] STM32 UART link lost.");
        }
    }
}

/**
 * @brief Process UART acknowledgments and battery messages without blocking.
 */
void Task_Process_UART_RX(void) {
    while (Serial1.available() > 0) {
        const int incoming = Serial1.read();
        if (incoming < 0) { break; }

        const char byte = static_cast<char>(incoming);
        if (byte == 'K') {
            if (!has_sent_uart_heartbeat) { continue; }

            const uint32_t acknowledgment_time = millis();
            if (!has_uart_acknowledgment ||
                acknowledgment_time - last_uart_acknowledgment > UART_ACK_TIMEOUT_MS) {
                uart_acknowledgment_streak = 0;
            }
            if (uart_acknowledgment_streak < 3) {
                ++uart_acknowledgment_streak;
            }
            last_uart_acknowledgment = acknowledgment_time;
            has_uart_acknowledgment = true;
            continue;
        }

        if (byte == '\r') { continue; }
        if (byte == '\n') {
            if (!uart_rx_discarding_line && uart_rx_line_length > 1 &&
                uart_rx_line[0] == 'B') {
                uart_rx_line[uart_rx_line_length] = '\0';
                rover_battery = String(&uart_rx_line[1]);
            }
            uart_rx_line_length = 0;
            uart_rx_discarding_line = false;
            continue;
        }

        if (uart_rx_discarding_line) { continue; }
        if (uart_rx_line_length == 0 && byte != 'B') { continue; }

        if (uart_rx_line_length < sizeof(uart_rx_line) - 1) {
            uart_rx_line[uart_rx_line_length++] = byte;
        } else {
            uart_rx_line_length = 0;
            uart_rx_discarding_line = true;
        }
    }
}
