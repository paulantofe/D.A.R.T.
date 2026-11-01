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
 * @brief  Initialize camera module of the esp32-cam board
 * @retval ESP_OK   Camera initialized successfully
 * @retval ESP_FAIL Camera initialization failed
 */
esp_err_t Camera_Init(void);

#endif