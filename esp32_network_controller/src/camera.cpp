/**
 * @file     camera.cpp
 * @brief    Source file for OV3660 camera module.
 * 
 * @details  This file provides the Camera_Init implementation and camera pin
 *           definitions for the AI-Thinker ESP32-CAM board.
 */

#include <Arduino.h>
#include "camera.h"

/**
 * @defgroup CAMERA_PINS
 * @{
 */
static constexpr int8_t PWDN_GPIO_NUM = 32;
static constexpr int8_t RESET_GPIO_NUM = -1;
static constexpr int8_t XCLK_GPIO_NUM = 0;
static constexpr int8_t SIOD_GPIO_NUM = 26;
static constexpr int8_t SIOC_GPIO_NUM = 27;

static constexpr int8_t Y9_GPIO_NUM = 35;
static constexpr int8_t Y8_GPIO_NUM = 34;
static constexpr int8_t Y7_GPIO_NUM = 39;
static constexpr int8_t Y6_GPIO_NUM = 36;
static constexpr int8_t Y5_GPIO_NUM = 21;
static constexpr int8_t Y4_GPIO_NUM = 19;
static constexpr int8_t Y3_GPIO_NUM = 18;
static constexpr int8_t Y2_GPIO_NUM = 5;
static constexpr int8_t VSYNC_GPIO_NUM = 25;
static constexpr int8_t HREF_GPIO_NUM = 23;
static constexpr int8_t PCLK_GPIO_NUM = 22;

static constexpr int8_t LED_GPIO_NUM = 4;
/** @} */

/* ------------------ Camera Initialization ------------------ */

/** Configure and initialize the camera sensor. */
esp_err_t Camera_Init(void) {
    camera_config_t config = {};
    
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    config.pin_xclk  = XCLK_GPIO_NUM;
    config.pin_pclk  = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href  = HREF_GPIO_NUM;

    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn  = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    config.xclk_freq_hz = 20000000;

    config.pixel_format = PIXFORMAT_JPEG; 
    config.grab_mode    = CAMERA_GRAB_LATEST;

    if (psramFound()) {
        config.frame_size   = FRAMESIZE_VGA; 
        config.jpeg_quality = 20;            
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        Serial.println("[CAMERA] PSRAM found. Using it for better quality");             
    } else {
        config.frame_size   = FRAMESIZE_CIF; 
        config.jpeg_quality = 18;
        config.fb_count     = 1;
        config.fb_location  = CAMERA_FB_IN_DRAM;
        Serial.println("[CAMERA] PSRAM not found. Lower quality");
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[CAMERA] Failed to initialize hardware. Error: 0x%x\n", err);
        return err;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor == nullptr) {
        Serial.println("[CAMERA] Sensor handle unavailable.");
        return ESP_FAIL;
    }

    sensor->set_vflip(sensor, 1);   // Vertical Flip
    // sensor->set_hmirror(sensor, 1); // Horizontal flip

    Serial.println("[CAMERA] OV3660 Initialized Successfully.");
    return ESP_OK;
}

/* ----------------------------------------------------------- */