#include <Arduino.h>
#include "app.h"


void setup() {
    ESP_Boot();
}

void loop() {
    Task_Process_UART_RX();
    uint32_t current_time = millis();

    Task_Network_Failsafe(current_time);
    Task_Hardware_Failsafe(current_time);

    delay(50);
}