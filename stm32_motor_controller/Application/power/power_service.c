/**
 * @file    power_service.c
 * @brief   INA219-based power-monitoring service implementation.
 * @details Owns the INA219 instance and translates driver measurements into
 *          application-level power and battery-monitoring APIs.
 */

#include "power_service.h"
#include "stm32f4xx_hal_hardware_config.h"
#include "ina219_driver.h"
#include "system_state_service.h"
#include <stdbool.h>

/* ------------------ Static Service Instances ------------------ */

#define P_LI_ION_THRESHOLD_V   6.0f
#define P_LI_ION_FULL_V        8.4f
#define P_CHECK_PERIOD_MS      1000U
#define P_PERCENT_FILTER_ALPHA_UP   0.20f
#define P_PERCENT_FILTER_ALPHA_DOWN 0.05f

static INA_Handle_t hina;
static bool is_initialized = false;
static uint32_t last_battery_check = 0U;
static bool filter_active = false;
static float previous_filtered_value = 0.0f;

/* ---------------------------------------------------------------- */

/* ------------------ Public APIs Implementation ------------------ */

HAL_StatusTypeDef P_Init(void) {
	HAL_StatusTypeDef status;

	is_initialized = false;
	status = I2C_Config();
	if (status != HAL_OK) {
		return status;
	}

	hina.config.hi2c = &hi2c2;
	hina.config.i2c_addr = INA_ADDR_GND_GND;
	hina.config.r_shunt_ohm = 0.1f;
	hina.config.max_current_a = 0.5f;
	hina.config.bus_range = INA_BRNG_16V;
	hina.config.pga_gain = INA_PGA_80MV;
	hina.config.bus_adc = INA_BADC_16S;
	hina.config.shunt_adc = INA_SADC_16S;
	hina.config.mode = INA_MODE_SHUNT_BUS_CONT;

    is_initialized = false;
	last_battery_check = 0U;
	filter_active = false;
	previous_filtered_value = 0.0f;

	status = INA_Init(&hina);
	if (status != HAL_OK) {
		SS_RequestState(SS_State_Systemfault);
		return status;
	}

	is_initialized = true;
	last_battery_check = HAL_GetTick() - P_CHECK_PERIOD_MS;

	return HAL_OK;
}

HAL_StatusTypeDef P_ReadBusVoltage_V(float *voltage) {
	if (!is_initialized || voltage == NULL) {
		return HAL_ERROR;
	}

	return INA_ReadBusVoltage_V(&hina, voltage);
}

HAL_StatusTypeDef P_ReadShuntVoltage_mV(float *shunt_voltage) {
	if (!is_initialized || shunt_voltage == NULL) {
		return HAL_ERROR;
	}

	return INA_ReadShuntVoltage_mV(&hina, shunt_voltage);
}

HAL_StatusTypeDef P_ReadCurrent_A(float *current) {
	if (!is_initialized || current == NULL) {
		return HAL_ERROR;
	}

	return INA_ReadCurrent_A(&hina, current);
}

HAL_StatusTypeDef P_ReadPower_W(float *power) {
	if (!is_initialized || power == NULL) {
		return HAL_ERROR;
	}

	return INA_ReadPower_W(&hina, power);
}

HAL_StatusTypeDef P_CheckBatteryVoltage(void) {
	HAL_StatusTypeDef status;
	uint32_t now;

	if (!is_initialized) {
		return HAL_ERROR;
	}

	now = HAL_GetTick();
	if ((now - last_battery_check) < P_CHECK_PERIOD_MS) {
		return HAL_OK;
	}

	last_battery_check = now;

	float voltage_reading = 0.0f;

	status = INA_ReadBusVoltage_V(&hina, &voltage_reading);

	if (status != HAL_OK) {
		return status;
	}

	if (voltage_reading <= P_LI_ION_THRESHOLD_V) {
		return SS_RequestState(SS_State_Systemfault);
	}

	return HAL_OK;
}

HAL_StatusTypeDef P_ReadBatteryPercent(float *percent) {
	HAL_StatusTypeDef status;
	float voltage_range;
	float new_percent;
	float filter_alpha;

	if (!is_initialized || percent == NULL) {
		return HAL_ERROR;
	}

	float voltage_reading = 0.0f;

	status = INA_ReadBusVoltage_V(&hina, &voltage_reading);
	if (status != HAL_OK) {
		return status;
	}

	voltage_range = P_LI_ION_FULL_V - P_LI_ION_THRESHOLD_V;
	if (voltage_reading <= P_LI_ION_THRESHOLD_V) {
		new_percent = 0.0f;
	} else if (voltage_reading >= P_LI_ION_FULL_V) {
		new_percent = 100.0f;
	} else {
		new_percent = ((voltage_reading - P_LI_ION_THRESHOLD_V)
				/ voltage_range) * 100.0f;
	}

	if (!filter_active) {
		filter_active = true;
		previous_filtered_value = new_percent;
	} else {
		filter_alpha = (new_percent > previous_filtered_value) ?
				P_PERCENT_FILTER_ALPHA_UP :
				P_PERCENT_FILTER_ALPHA_DOWN;
		previous_filtered_value +=
			filter_alpha * (new_percent - previous_filtered_value);
	}

	*percent = previous_filtered_value;

	return HAL_OK;
}

/* ---------------------------------------------------------------- */
