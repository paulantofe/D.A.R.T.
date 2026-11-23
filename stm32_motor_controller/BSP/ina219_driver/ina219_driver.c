#include "ina219_driver.h"
#include <stdbool.h>

#define INA_I2C_TIMEOUT_MS      10U
#define INA_SLAVE_TRIES         3U
#define INA_PAYLOAD_LENGTH      2      // bytes

/* ------------------ Static Helper Functions ------------------ */

static inline bool is_valid_addr(uint8_t addr) {
	return (addr == INA_ADDR_GND_GND || addr == INA_ADDR_GND_SCL ||
			addr == INA_ADDR_GND_SDA || addr == INA_ADDR_GND_VS_P);
}

static inline bool is_valid_reg(uint8_t reg) {
	return (reg == INA_CONFIGURATION_REG_ADDR ||
			reg == INA_SHUNT_VOLTAGE_REG_ADDR ||
			reg == INA_BUS_VOLTAGE_REG_ADDR ||
			reg == INA_POWER_REG_ADDR ||
			reg == INA_CURRENT_REG_ADDR ||
			reg == INA_CALIBRATION_REG_ADDR);
}

static HAL_StatusTypeDef INA_WriteReg(INA_Handle_t *hina, uint8_t reg, uint16_t reg_value) {
	if (hina == (void*)0 || hina->hi2c == (void*)0 || !is_valid_reg(reg)) {
		return HAL_ERROR;
	}

	uint8_t buf[INA_PAYLOAD_LENGTH];
	buf[0] = (uint8_t)((reg_value >> 8) & 0xFFU);
	buf[1] = (uint8_t)(reg_value & 0xFFU);

	return HAL_I2C_Mem_Write(hina->hi2c, hina->i2c_addr, reg,
				I2C_MEMADD_SIZE_8BIT, buf, INA_PAYLOAD_LENGTH, INA_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef INA_ReadReg(INA_Handle_t *hina, uint8_t reg, uint16_t *reg_value) {
	if (hina == (void*)0 || hina->hi2c == (void*)0 || !is_valid_reg(reg) || reg_value == (void*)0) {
		return HAL_ERROR;
	}

	uint8_t buf[INA_PAYLOAD_LENGTH];
	HAL_StatusTypeDef status = HAL_I2C_Mem_Read(hina->hi2c, hina->i2c_addr, reg,
			I2C_MEMADD_SIZE_8BIT, buf, INA_PAYLOAD_LENGTH, INA_I2C_TIMEOUT_MS);

	if (status == HAL_OK) {
		*reg_value = ((uint16_t)buf[0] << 8) | buf[1];
	}

	return status;
}

/* ------------------------------------------------------------- */


/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef INA_Init(INA_Handle_t *hina, I2C_HandleTypeDef *hi2c, uint8_t addr) {
	if (hina == (void*)0 || hi2c == (void*)0 ||
			!is_valid_addr(addr)) {
		return HAL_ERROR;
	}

	hina->hi2c = hi2c;
	hina->i2c_addr = addr;
	hina->cal_value = 0U;
	hina->current_lsb = 0.0f;
	hina->power_lsb = 0.0f;

	HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(hina->hi2c, hina->i2c_addr,
			INA_SLAVE_TRIES, INA_I2C_TIMEOUT_MS);
	if (status != HAL_OK) {
		return status;
	}

	status = INA_WriteReg(hina, INA_CONFIGURATION_REG_ADDR, (1U << INA_CFGR_RST));
	if (status != HAL_OK) {
		return status;
	}

	return HAL_OK;
}

/* ---------------------------------------------------------------- */
