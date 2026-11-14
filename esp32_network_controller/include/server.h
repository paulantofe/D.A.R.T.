/**
 * @file     server.h
 * @brief    Header file for server support.
 * 
 * @details  This file provides the API for the ESP32 web server, including:
 *              - /stream for the camera MJPEG stream
 *              - /control for rover control commands
 *              - /status for battery, Wi-Fi, link, mode, and stream-client data
 *              - mDNS access through http://dart.local
 *              - Wi-Fi configuration through the WiFiManager access point
 * 
 * @note     The server first attempts to connect to the last known Wi-Fi
 *           network. If that fails, it enables an access point for 3 minutes
 *           before restarting.
 * @note     Only one client may control the rover at a time. Other clients
 *           remain spectators. The driver is released after 2 seconds without
 *           a /status request.
 *           
 */

#ifndef SERVER_H
#define SERVER_H

#include "esp_camera.h"

/**
 * @brief  Initialize the ESP32 web server.
 * @retval ESP_OK   Server initialized successfully
 * @retval ESP_FAIL Server initialization failed
 */
esp_err_t Server_Init(void);

/**
 * @brief  Stop the ESP32 web server and Wi-Fi.
 */
void Server_Stop(void);

#endif
