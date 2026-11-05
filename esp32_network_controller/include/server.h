/**
 * @file     server.h
 * @brief    Header file for server support.
 * 
 * @details  This file provides Server_Init API which configures the ESP32 Server for Rover Control Web Client supporting:
 *              - /stream web route for live stream from on-board camera
 *              - /control web route for live rover control
 *              - /status web route for status JSON packages containing battery, wifi and mode status,
 *           as well as WiFi connection:
 *              - mDNS for easy access on http://dart.local
 *              - easy rover wifi configuration via Access Point.
 * 
 * @note     The server is configured to connect to the last known WiFi. Only after it fails, it enables AP mode to configure the 
 *           new WiFi connection, giving a 3 minute window before restarting.
 *           The server accepts only one driver (Web Browser controlling the rover). Any other client is automatically in spectator
 *           mode (can see the live stream and status, but not control). Driver auto-disconnect is triggered after 2.5 seconds with
 *           no browser request.
 *           
 */

#ifndef SERVER_H
#define SERVER_H

#include <Arduino.h>
#include <stdint.h>
#include "esp_camera.h" 

/**
 * @brief  Initialize esp32 web server
 * @retval ESP_OK   Server initialized successfully
 * @retval ESP_FAIL Server initialization failed
 */
esp_err_t Server_Init(void);

/**
 * @brief  Stop esp32 web server and WiFi
 */
void Server_Stop(void);

#endif