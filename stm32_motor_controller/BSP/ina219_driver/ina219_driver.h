/**
 * @file    ina219_driver.h
 * @brief   Public API, register definitions, and data structures for the INA219 sensor driver.
 * @details Provides the hardware-independent interface for initializing, calibrating, and
 *          reading bus voltage, shunt voltage, current, and power measurements from the
 *          Texas Instruments INA219 high-side current/power monitor over I2C using STM32 HAL.
 *
 * @note    The I2C communication is using the blocking APIs. The given INA APIs are therefore
 *          expected to be called from non-ISR context.
 */

#ifndef INA219_DRIVER_INA219_DRIVER_H_
#define INA219_DRIVER_INA219_DRIVER_H_

#include "stm32f4xx_hal_hardware_config.h"
#include <stdint.h>

/* ------------------ General Macros Definition ------------------ */

// Handle structure for INA sensor.
typedef struct {
	I2C_HandleTypeDef *hi2c;
	uint8_t i2c_addr;
	uint16_t cal_value;
	float current_lsb;
	float power_lsb;
} INA_Handle_t;

/**
 * @defgroup INA_REG_ADDRS           Register Addresses of INA219
 * @{
 */
#define INA_CONFIGURATION_REG_ADDR   0x00U
#define INA_SHUNT_VOLTAGE_REG_ADDR   0x01U
#define INA_BUS_VOLTAGE_REG_ADDR     0x02U
#define INA_POWER_REG_ADDR           0x03U
#define INA_CURRENT_REG_ADDR         0x04U
#define INA_CALIBRATION_REG_ADDR     0x05U
/** @} */

/**
 * @defgroup INA_SLAVE_ADDRS         Available Slave Addresses of INA219
 * @details  Addresses are selected by physically driving the A0 and A1 pins to the
 *           corresponding signal source.
 * @note     This driver defines only the first four combinations.
 * @{
 */
#define INA_ADDR_GND_GND             (0x40U << 1)
#define INA_ADDR_GND_VS_P            (0x41U << 1)
#define INA_ADDR_GND_SDA             (0x42U << 1)
#define INA_ADDR_GND_SCL             (0x43U << 1)
/** @} */

/**
 * @defgroup INA_PIN_DEF             Pin Definitions of INA Registers
 * @{
 */
// Configuration Register Bits
#define INA_CFGR_RST                 (1U << 15U)
#define INA_CFGR_BRNG                (1U << 13U)
#define INA_CFGR_PG1                 (1U << 12U)
#define INA_CFGR_PG0                 (1U << 11U)
#define INA_CFGR_BADC4               (1U << 10U)
#define INA_CFGR_BADC3               (1U << 9U)
#define INA_CFGR_BADC2               (1U << 8U)
#define INA_CFGR_BADC1               (1U << 7U)
#define INA_CFGR_SADC4               (1U << 6U)
#define INA_CFGR_SADC3               (1U << 5U)
#define INA_CFGR_SADC2               (1U << 4U)
#define INA_CFGR_SADC1               (1U << 3U)
#define INA_CFGR_MODE3               (1U << 2U)
#define INA_CFGR_MODE2               (1U << 1U)
#define INA_CFGR_MODE1               (1U << 0U)

// Bus Voltage Register Bits
#define INA_BVR_BD_POS               3U
#define INA_BVR_CNVR                 (1U << 1U)
#define INA_BVR_OVF                  (1U << 0U)

// Bus Voltage Register Masks
#define INA_BVR_BD_MASK              0xFFF8U
#define INA_BVR_CNVR_MASK            INA_BVR_CNVR
#define INA_BVR_OVF_MASK             INA_BVR_OVF

// Calibration Register Bits
#define INA_CAL_FS0_MASK             0xFFFEU
/** @} */

/* --------------------------------------------------------------- */


/* ------------------ Public APIs ------------------ */

/**
 * @brief Initialize the INA219.
 * @param[in,out] hina  pointer to INA handle structure.
 * @param[in]     hi2c  pointer to I2C handle structure.
 * @param[in]     addr  address of the INA slave.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_Init(INA_Handle_t *hina, I2C_HandleTypeDef *hi2c, uint8_t addr);

/**
 * @brief Calibrate the INA219.
 * @param[in] hina           pointer to INA handle structure.
 * @param[in] r_shunt_ohm    value of INA shunt resistor in ohms.
 * @param[in] max_current_a  expected max current in amperes.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_Calibrate(INA_Handle_t *hina, float r_shunt_ohm, float max_current_a);

/**
 * @brief Read the shunt voltage.
 * @param[in]  hina      pointer to INA handle structure.
 * @param[out] shunt_mv  shunt voltage drop over the shunt resistor in millivolts.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_ReadShuntVoltage_mV(INA_Handle_t *hina, float *shunt_mv);

/**
 * @brief Read the bus voltage.
 * @param[in]  hina     pointer to INA handle structure.
 * @param[out] voltage  voltage read by INA.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_ReadBusVoltage_V(INA_Handle_t *hina, float *voltage);

/**
 * @brief Read the current flowing through INA.
 * @param[in]  hina     pointer to INA handle structure.
 * @param[out] current  current read by INA.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_ReadCurrent_A(INA_Handle_t *hina, float *current);

/**
 * @brief Read the power flowing through INA.
 * @param[in]  hina   pointer to INA handle structure.
 * @param[out] power  power read by INA.
 * @return HAL_OK    Operation Successful.
 *         HAL_ERROR Something went wrong.
 */
HAL_StatusTypeDef INA_ReadPower_W(INA_Handle_t *hina, float *power);

/* ------------------------------------------------- */

#endif
