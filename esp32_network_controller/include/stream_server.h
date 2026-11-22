/**
 * @file    stream_server.h
 * @brief   Dedicated MJPEG stream server interface (port 81).
 */

#ifndef STREAM_SERVER_H
#define STREAM_SERVER_H

#include <stdint.h>
#include "esp_err.h"

/* ------------------ Public API ------------------ */

/**
 * @brief    Initialize and start the MJPEG stream server.
 * @return   ESP_OK on success, or an esp_err_t error code on failure.
 */
esp_err_t StreamServer_Init(void);

/**
 * @brief    Stop the MJPEG stream server.
 */
void StreamServer_Stop(void);

/**
 * @brief    Get the number of active stream clients.
 * @return   The number of active stream clients.
 */
uint32_t StreamServer_GetActiveClients(void);

/* ------------------------------------------------ */

#endif