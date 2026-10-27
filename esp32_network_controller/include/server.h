#ifndef SERVER_H
#define SERVER_H

#include <Arduino.h>
#include "esp_camera.h" 

/**
 * @brief  Initialize esp32 server
 * @retval ESP_OK/ESP_FAIL macro
 */
esp_err_t Server_Init(void);

#endif