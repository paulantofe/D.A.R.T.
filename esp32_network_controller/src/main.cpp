#include <Arduino.h>
#include "globals.h"
#include "server.h"
#include "camera.h"

String rover_battery = "N/A";
volatile uint32_t last_command_time = 0;
volatile bool has_active_client = false;

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
    if (millis() - last_command_time > 2500) {
        if (has_active_client) {
            has_active_client = false;
            Serial.println("[SERVER] Driver disconnected (Timeout elapsed)");
            Serial1.print('0');
        }
    }   

    delay(50);
}