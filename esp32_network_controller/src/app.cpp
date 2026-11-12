/**
 * @file     app.cpp
 * @brief    ESP32 application runtime implementation.
 */

#include <Arduino.h>
#include "app.h"
#include "globals.h"
#include "camera.h"
#include "server.h"
#include "serial_protocol.h"

/* ------------------ System Fault Handler ------------------ */

static constexpr uint32_t SEND_X_FREQ_MS = 50U;

void SystemFault_Enter(void) {
    system_state = System_State_t::SystemFault;

    Serial.println("[SYSTEM] Encountered System Fault. Please reboot the rover.");
    
    while (true) {
        Serial1.write(SP_SYSTEM_FAULT);
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


/* ------------------ Handshake Handler ------------------ */

static constexpr uint8_t REQUIRED_R_COUNT = 3U;
static constexpr uint32_t R_INTERVAL_TIMEOUT_MS = 100U;

static uint8_t r_count;
static uint32_t last_r_time;
static bool reset_handshake = true;
static bool boot_handshake = true;

void LinkHandshake_ProcessByte(const char received, const uint32_t now) {
    if (received != SP_STM32_HEARTBEAT) { return; }
    
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
        Serial1.write(SP_UART_HEARTBEAT);
        Serial1.write(has_active_client
                          ? SP_DRIVER_CONNECTED
                          : SP_DRIVER_DISCONNECTED);

        if (boot_handshake) {
            boot_handshake = false;

            Serial.println("[SYSTEM] STM32 Connected. UART Link is online.");
        }
    }
}

void LinkHandshake_CheckTimeout(const uint32_t now) {
    if (link_state == LINK_ONLINE && now - last_r_time > R_INTERVAL_TIMEOUT_MS) {
        link_state = LINK_OFFLINE;
        reset_handshake = true;
        r_count = 0;
    }

    if (link_state == LINK_HANDSHAKING && now - last_r_time > R_INTERVAL_TIMEOUT_MS) {
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


/* ------------------ Battery Handler ------------------ */

/** Minimum 2S battery-pack voltage used for percentage conversion. */
static constexpr float BATTERY_MIN_VOLTAGE = 6.0f;

/** Maximum 2S battery-pack voltage used for percentage conversion. */
static constexpr float BATTERY_MAX_VOLTAGE = 8.4f;

/** Maximum interval between bytes in one battery frame. */
static constexpr uint32_t BATTERY_FRAME_TIMEOUT_MS = 100U;

/** Maximum number of characters in the voltage payload. */
static constexpr uint8_t BATTERY_BUFFER_MAX_LEN = 4U;

/** Accumulated voltage payload between B and the terminating newline. */
static String received_battery_buffer = "";

/** Indicates that the current battery frame has received its newline. */
static bool battery_reception_done = false;

/** Indicates that Serial1 is currently receiving a battery frame. */
static bool receiving_battery_frame = false;

/** Timestamp of the most recently received battery-frame byte. */
static volatile uint32_t last_battery_byte = 0;

/** Number of voltage-payload characters received in the current frame. */
static uint8_t buffer_length = 0;

/**
 * @brief    Store one byte from the current battery frame.
 * @details  Aborts the frame when the inter-byte timeout or payload limit is
 *           exceeded, and marks the frame complete when a newline is found.
 */
void Battery_ProcessByte(const char received) {
    if (millis() - last_battery_byte > BATTERY_FRAME_TIMEOUT_MS) {
        receiving_battery_frame = false;
        battery_reception_done = false;
        received_battery_buffer = "";
        buffer_length = 0;
        return;
    }

    if (received == '\n') {
        battery_reception_done = true;
        return;
    }

    if (buffer_length >= BATTERY_BUFFER_MAX_LEN) {
        receiving_battery_frame = false;
        battery_reception_done = false;
        received_battery_buffer = "";
        buffer_length = 0;
        return;
    }

    last_battery_byte = millis();
    buffer_length++;
    received_battery_buffer += received;
}

/**
 * @brief    Convert the received battery voltage to a percentage string.
 * @details  Uses the configured 2S pack limits and clamps the result to
 *           0–100 percent. An invalid or non-positive reading returns empty.
 */
String Battery_ParseBuffer(void) {
    const float voltage = received_battery_buffer.toFloat();

    if (voltage <= 0.0f) {
        return "";
    }

    float voltage_percent =
        ((voltage - BATTERY_MIN_VOLTAGE) /
         (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE)) * 100.0f;

    if (voltage_percent < 0.0f) {
        voltage_percent = 0.0f;
    } else if (voltage_percent > 100.0f) {
        voltage_percent = 100.0f;
    }

    return String(voltage_percent);
}

/**
 * @brief    Reset the state and begin receiving a battery frame.
 */
void Battery_StartFrame(void) {
    receiving_battery_frame = true;
    received_battery_buffer = "";
    battery_reception_done = false;
    last_battery_byte = millis();
    buffer_length = 0;
}

/**
 * @brief    Process a byte and publish a completed battery frame.
 */
void Battery_ProcessFrame(const char received) {
    Battery_ProcessByte(received);

    if (battery_reception_done) {
        battery_reception_done = false;

        String parse_battery = Battery_ParseBuffer();
        if (parse_battery != "") {
            battery_level = parse_battery;
        }

        received_battery_buffer = "";
        receiving_battery_frame = false;
    }
}

/* -------------------------------------------------- */


/* ------------------ Serial1 Handler ------------------ */

void Serial1_Process(void) {
    while (Serial1.available() > 0) {
        const char received = Serial1.read();
        const uint32_t now = millis();

        if (receiving_battery_frame) {
            Battery_ProcessFrame(received);
        } else if (received == SP_STM32_HEARTBEAT) {
            LinkHandshake_ProcessByte(received, now);
        } else if (received == SP_BATTERY_FRAME) {
            Battery_StartFrame();
        }
    }
}

/* ----------------------------------------------------- */


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