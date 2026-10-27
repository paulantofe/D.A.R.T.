#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h" 

/**
 * @brief  Initialize camera module of the esp32-cam board
 * @retval ESP_OK/ESP_FAIL macro
 */
esp_err_t Camera_Init(void);

#endif