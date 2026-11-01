/**
 * @file     main.cpp
 * @brief    Main project source file.
 * 
 * @details  This file initializes server and camera module. If any of these two fails, it triggers System_Fault state.
 */

#include <Arduino.h>
#include "globals.h"
#include "server.h"
#include "camera.h"

/* -------------- Global Variables Initialization -------------- */

String rover_battery = "N/A";
volatile uint32_t last_command_time = 0;
volatile bool has_active_client = false;

/* ------------------------------------------------------------- */

void setup() {
    Serial.begin(115200);

    if (Camera_Init() != ESP_OK) {
        Serial.println("Camera Init Failed");
    }

    if (Server_Init() != ESP_OK) {
        Serial.println("Server Init failed");
    }
}

void loop() {
    // Check driver browser inactivity
    if (millis() - last_command_time > 2500) {
        if (has_active_client) {
            // Let other browser become driver and stop rover
            has_active_client = false;
            Serial.println("[SERVER] Driver disconnected (Timeout elapsed)");
            Serial1.print('F');
        }
    }   

    delay(50);
}