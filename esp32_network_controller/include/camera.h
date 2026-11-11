/**
 * @file     camera.h
 * @brief    Header file for OV3660 camera module.
 * 
 * @details  This file provides Camera_Init API which configures the camera for the AI-Thinker ESP32 Board.
 *           Camera pins definitions are found in camera.cpp source file for encapsulation reasons.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <Arduino.h>
#include "esp_camera.h" 

/**
 * @brief    Initialize the camera module of the ESP32-CAM board.
 * @retval   ESP_OK  Camera initialized successfully.
 * @retval   Other   An ESP error code returned by the camera driver.
 */
esp_err_t Camera_Init(void);

#endif