/**
 * @file    power_service.h
 * @brief   Public API for the rover power-monitoring service.
 * @details Provides access to INA219 bus-voltage, shunt-voltage, current,
 *          power, battery-voltage monitoring, and battery-percentage estimates.
 *
 * @note    The service owns its INA219 driver instance and must be initialized
 *          before any measurement API is called.
 */

#ifndef POWER_POWER_SERVICE_H_
#define POWER_POWER_SERVICE_H_

#include "stm32f4xx_hal.h"

/* ------------------ Public Power APIs ------------------ */

/**
 * @brief Initialize the power-monitoring service.
 * @details Configures I2C2 and initializes the INA219 with the rover's shunt,
 *          current, range, ADC, and operating-mode settings.
 * @retval HAL_OK       Power service initialized successfully.
 * @retval HAL_ERROR    Configuration or INA219 initialization failed.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 */
HAL_StatusTypeDef P_Init(void);

/**
 * @brief Read the voltage across the INA219 shunt resistor.
 * @param[out] shunt_voltage Destination for the shunt voltage in millivolts.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    Service is uninitialized or the argument is invalid.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 */
HAL_StatusTypeDef P_ReadShuntVoltage_mV(float *shunt_voltage);

/**
 * @brief Read the monitored bus voltage.
 * @param[out] voltage Destination for the bus voltage in volts.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    Service is uninitialized or the argument is invalid.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 */
HAL_StatusTypeDef P_ReadBusVoltage_V(float *voltage);

/**
 * @brief Read the monitored current.
 * @param[out] current Destination for the current in amperes.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    Service is uninitialized or the argument is invalid.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 */
HAL_StatusTypeDef P_ReadCurrent_A(float *current);

/**
 * @brief Read the monitored power.
 * @param[out] power Destination for the power in watts.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    Service is uninitialized or the argument is invalid.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 */
HAL_StatusTypeDef P_ReadPower_W(float *power);

/**
 * @brief Check the battery voltage against the configured safety threshold.
 * @details The check is rate-limited internally. When the measured voltage is
 *          at or below the threshold, a system-fault state is requested.
 * @retval HAL_OK       Voltage is acceptable or the check interval has not elapsed.
 * @retval HAL_ERROR    Service is uninitialized or the state request failed.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 * @note    Call this API periodically from the application super loop.
 *          Call @ref SS_Process separately to apply the requested state.
 */
HAL_StatusTypeDef P_CheckBatteryVoltage(void);

/**
 * @brief Estimate the battery charge percentage from bus voltage.
 * @param[out] percent Destination for the estimated percentage in the range
 *                     0.0 to 100.0.
 * @retval HAL_OK       Estimate calculated successfully.
 * @retval HAL_ERROR    Service is uninitialized or the argument is invalid.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  INA219 communication timed out.
 * @note    This is a filtered, linear voltage-based estimate for the
 *          configured battery range, not a coulomb-counted state-of-charge
 *          measurement.
 */
HAL_StatusTypeDef P_ReadBatteryPercent(float *percent);

/* ------------------------------------------------------- */

#endif /* POWER_POWER_SERVICE_H_ */
