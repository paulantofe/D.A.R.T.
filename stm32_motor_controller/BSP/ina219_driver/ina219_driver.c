/**
 * @file    ina219_driver.c
 * @brief   Implementation of the INA219 current and power monitor driver.
 * @details Implements blocking I2C register read/write operations (including Big-Endian to
 *          Little-Endian byte swapping), device initialization, calibration register scaling,
 *          and raw-to-physical unit conversions for STM32F4 using STM32 HAL.
 */

#include "ina219_driver.h"
#include <stdbool.h>

#define INA_I2C_TIMEOUT_MS       10U
#define INA_SLAVE_TRIES          3U
#define INA_PAYLOAD_LENGTH       2        // bytes
#define INA_SHUNT_VOLTAGE_LSB    0.01f
#define INA_VOLTAGE_LSB          0.004f
#define INA_CAL_CURRENT_DIVISOR  32767.0f // used to calculate current_lsb
#define INA_CAL_POWER_MULTIPLIER 20.0f    // used to calculate power_lsb
#define INA_COMPUTE_CALIBRATION_VALUE(c, r) \
	(0.04096f / ((c) * (r)))

/* ------------------ Static Helper Functions ------------------ */

static inline bool is_valid_addr(const uint8_t addr) {
	return (addr == INA_ADDR_GND_GND || addr == INA_ADDR_GND_SCL ||
			addr == INA_ADDR_GND_SDA || addr == INA_ADDR_GND_VS_P);
}

static inline bool is_valid_reg(const uint8_t reg) {
	return (reg == INA_CONFIGURATION_REG_ADDR ||
			reg == INA_SHUNT_VOLTAGE_REG_ADDR ||
			reg == INA_BUS_VOLTAGE_REG_ADDR ||
			reg == INA_POWER_REG_ADDR ||
			reg == INA_CURRENT_REG_ADDR ||
			reg == INA_CALIBRATION_REG_ADDR);
}

static HAL_StatusTypeDef INA_WriteReg(const INA_Handle_t *hina, const uint8_t reg, const uint16_t reg_value) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		!is_valid_reg(reg) || !is_valid_addr(hina->config.i2c_addr)) {
		return HAL_ERROR;
	}

	uint8_t buf[INA_PAYLOAD_LENGTH];
	buf[0] = (uint8_t)((reg_value >> 8) & 0xFFU);
	buf[1] = (uint8_t)(reg_value & 0xFFU);

	return HAL_I2C_Mem_Write(hina->config.hi2c, hina->config.i2c_addr, reg,
				I2C_MEMADD_SIZE_8BIT, buf, INA_PAYLOAD_LENGTH, INA_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef INA_ReadReg(const INA_Handle_t *hina, const uint8_t reg, uint16_t *reg_value) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		!is_valid_reg(reg) || reg_value == (void*)0 ||
		!is_valid_addr(hina->config.i2c_addr)) {
		return HAL_ERROR;
	}

	uint8_t buf[INA_PAYLOAD_LENGTH];
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hina->config.hi2c, hina->config.i2c_addr, reg,
			I2C_MEMADD_SIZE_8BIT, buf, INA_PAYLOAD_LENGTH, INA_I2C_TIMEOUT_MS);

	if (status == HAL_OK) {
		*reg_value = ((uint16_t)buf[0] << 8) | buf[1];
	}

	return status;
}

static HAL_StatusTypeDef INA_Calibrate(INA_Handle_t *hina) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		hina->config.r_shunt_ohm <= 0.0f ||
		hina->config.max_current_a <= 0.0f) {
		return HAL_ERROR;
	}

	float current_lsb = hina->config.max_current_a / INA_CAL_CURRENT_DIVISOR;
	float calibration_value = INA_COMPUTE_CALIBRATION_VALUE(current_lsb, hina->config.r_shunt_ohm);

	if (calibration_value < 2.0f || calibration_value > 65534.0f) {
		return HAL_ERROR;
	}

	uint16_t cal_reg_value = ((uint16_t)calibration_value) & INA_CAL_FS0_MASK;

	HAL_StatusTypeDef status = INA_WriteReg(hina, INA_CALIBRATION_REG_ADDR, cal_reg_value);
	if (status != HAL_OK) {
		return status;
	}

	hina->current_lsb = current_lsb;
	hina->power_lsb   = INA_CAL_POWER_MULTIPLIER * current_lsb;
	hina->cal_value   = cal_reg_value;

	return HAL_OK;
}

static HAL_StatusTypeDef INA_Reset(const INA_Handle_t *hina) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		!is_valid_addr(hina->config.i2c_addr)) {
		return HAL_ERROR;
	}

	HAL_StatusTypeDef status = INA_WriteReg(hina, INA_CONFIGURATION_REG_ADDR, INA_CFGR_RST);

	return status;
}

/* ------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef INA_Init(INA_Handle_t *hina) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		!is_valid_addr(hina->config.i2c_addr)) {
		return HAL_ERROR;
	}

	hina->cal_value = 0U;
	hina->current_lsb = 0.0f;
	hina->power_lsb = 0.0f;

	HAL_StatusTypeDef status =
			HAL_I2C_IsDeviceReady(
					   hina->config.hi2c,
					   hina->config.i2c_addr,
					   INA_SLAVE_TRIES,
					   INA_I2C_TIMEOUT_MS
					);

	if (status != HAL_OK) {
		return status;
	}

	status = INA_Reset(hina);
	if (status != HAL_OK) {
		return status;
	}

	uint16_t cfg_reg = hina->config.bus_range |
			           hina->config.pga_gain  |
			           hina->config.bus_adc   |
			           hina->config.shunt_adc |
			           hina->config.mode;

	status = INA_WriteReg(hina, INA_CONFIGURATION_REG_ADDR, cfg_reg);
	if (status != HAL_OK) {
		return status;
	}

	status = INA_Calibrate(hina);
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

HAL_StatusTypeDef INA_ReadShuntVoltage_mV(const INA_Handle_t *hina, float *shunt_mv) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 || shunt_mv == (void*)0) {
		return HAL_ERROR;
	}

	uint16_t shunt_voltage_raw = 0U;

	HAL_StatusTypeDef status = INA_ReadReg(hina, INA_SHUNT_VOLTAGE_REG_ADDR, &shunt_voltage_raw);
	if (status != HAL_OK) {
		return status;
	}

    *shunt_mv = (float)((int16_t)shunt_voltage_raw) * INA_SHUNT_VOLTAGE_LSB;

    return HAL_OK;
}

HAL_StatusTypeDef INA_ReadBusVoltage_V(const INA_Handle_t *hina, float *voltage) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 || voltage == (void*)0) {
		return HAL_ERROR;
	}

	uint16_t voltage_raw = 0U;

	HAL_StatusTypeDef status = INA_ReadReg(hina, INA_BUS_VOLTAGE_REG_ADDR, &voltage_raw);
	if (status != HAL_OK) {
		return status;
	}

	*voltage = (float)(voltage_raw >> INA_BVR_BD_POS) * INA_VOLTAGE_LSB;

	return HAL_OK;
}

HAL_StatusTypeDef INA_ReadCurrent_A(const INA_Handle_t *hina, float *current) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		current == (void*)0 || hina->cal_value == 0U) {
		return HAL_ERROR;
	}

	uint16_t current_raw = 0U;

	HAL_StatusTypeDef status = INA_ReadReg(hina, INA_CURRENT_REG_ADDR, &current_raw);
	if (status != HAL_OK) {
		return status;
	}

	*current = (float)((int16_t)current_raw) * hina->current_lsb;

	return HAL_OK;
}

HAL_StatusTypeDef INA_ReadPower_W(const INA_Handle_t *hina, float *power) {
	if (hina == (void*)0 || hina->config.hi2c == (void*)0 ||
		power == (void*)0 || hina->cal_value == 0U) {
		return HAL_ERROR;
	}

	uint16_t power_raw = 0U;

	HAL_StatusTypeDef status = INA_ReadReg(hina, INA_POWER_REG_ADDR, &power_raw);
	if (status != HAL_OK) {
		return status;
	}

	*power = (float)power_raw * hina->power_lsb;

	return HAL_OK;
}

/* ---------------------------------------------------------------- */
