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
 * @defgroup INA_BUS_RANGE           Bus Voltage Range (BRNG)
 * @{
 */
#define INA_BRNG_16V                 (0x0U << 13U) /**< 16V FSR (Full-Scale Range) */
#define INA_BRNG_32V                 (0x1U << 13U) /**< 32V FSR (Default) */
/** @} */

/**
 * @defgroup INA_PGA_GAIN            Shunt Voltage PGA Gain (PG1:PG0)
 * @{
 */
#define INA_PGA_40MV                 (0x0U << 11U) /**< Gain 1,   Range +/- 40 mV */
#define INA_PGA_80MV                 (0x1U << 11U) /**< Gain /2,  Range +/- 80 mV */
#define INA_PGA_160MV                (0x2U << 11U) /**< Gain /4,  Range +/- 160 mV */
#define INA_PGA_320MV                (0x3U << 11U) /**< Gain /8,  Range +/- 320 mV (Default) */
/** @} */

/**
 * @defgroup INA_BUS_ADC             Bus ADC Resolution / Averaging (BADC4:BADC1)
 * @{
 */
#define INA_BADC_9BIT                (0x0U << 7U)  /**< 9-bit,  1 sample  (84 us) */
#define INA_BADC_10BIT               (0x1U << 7U)  /**< 10-bit, 1 sample  (148 us) */
#define INA_BADC_11BIT               (0x2U << 7U)  /**< 11-bit, 1 sample  (276 us) */
#define INA_BADC_12BIT               (0x3U << 7U)  /**< 12-bit, 1 sample  (532 us, Default) */
#define INA_BADC_2S                  (0x9U << 7U)  /**< 12-bit, 2 samples averaging   (1.06 ms) */
#define INA_BADC_4S                  (0xAU << 7U)  /**< 12-bit, 4 samples averaging   (2.13 ms) */
#define INA_BADC_8S                  (0xBU << 7U)  /**< 12-bit, 8 samples averaging   (4.26 ms) */
#define INA_BADC_16S                 (0xCU << 7U)  /**< 12-bit, 16 samples averaging  (8.51 ms) */
#define INA_BADC_32S                 (0xDU << 7U)  /**< 12-bit, 32 samples averaging  (17.02 ms) */
#define INA_BADC_64S                 (0xEU << 7U)  /**< 12-bit, 64 samples averaging  (34.05 ms) */
#define INA_BADC_128S                (0xFU << 7U)  /**< 12-bit, 128 samples averaging (68.10 ms) */
/** @} */

/**
 * @defgroup INA_OPERATING_MODE      Operating Mode (MODE3:MODE1)
 * @{
 */
#define INA_MODE_POWER_DOWN          (0x0U << 0U)  /**< Power-down */
#define INA_MODE_SHUNT_TRIG          (0x1U << 0U)  /**< Shunt voltage, triggered */
#define INA_MODE_BUS_TRIG            (0x2U << 0U)  /**< Bus voltage, triggered */
#define INA_MODE_SHUNT_BUS_TRIG      (0x3U << 0U)  /**< Shunt and bus, triggered */
#define INA_MODE_ADC_OFF             (0x4U << 0U)  /**< ADC off (disabled) */
#define INA_MODE_SHUNT_CONT          (0x5U << 0U)  /**< Shunt voltage, continuous */
#define INA_MODE_BUS_CONT            (0x6U << 0U)  /**< Bus voltage, continuous */
#define INA_MODE_SHUNT_BUS_CONT      (0x7U << 0U)  /**< Shunt and bus, continuous (Default) */
/** @} */

/**
 * @defgroup INA_SHUNT_ADC           Shunt ADC Resolution / Averaging (SADC4:SADC1)
 * @{
 */
#define INA_SADC_9BIT                (0x0U << 3U)  /**< 9-bit,  1 sample  (84 us) */
#define INA_SADC_10BIT               (0x1U << 3U)  /**< 10-bit, 1 sample  (148 us) */
#define INA_SADC_11BIT               (0x2U << 3U)  /**< 11-bit, 1 sample  (276 us) */
#define INA_SADC_12BIT               (0x3U << 3U)  /**< 12-bit, 1 sample  (532 us, Default) */
#define INA_SADC_2S                  (0x9U << 3U)  /**< 12-bit, 2 samples averaging   (1.06 ms) */
#define INA_SADC_4S                  (0xAU << 3U)  /**< 12-bit, 4 samples averaging   (2.13 ms) */
#define INA_SADC_8S                  (0xBU << 3U)  /**< 12-bit, 8 samples averaging   (4.26 ms) */
#define INA_SADC_16S                 (0xCU << 3U)  /**< 12-bit, 16 samples averaging  (8.51 ms) */
#define INA_SADC_32S                 (0xDU << 3U)  /**< 12-bit, 32 samples averaging  (17.02 ms) */
#define INA_SADC_64S                 (0xEU << 3U)  /**< 12-bit, 64 samples averaging  (34.05 ms) */
#define INA_SADC_128S                (0xFU << 3U)  /**< 12-bit, 128 samples averaging (68.10 ms) */
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


/* ------------------ INA Sensor Structures ------------------ */

/**
 * @brief User configuration structure for the INA219 sensor.
 * @details Must be populated by the user before calling @ref INA_Init.
 */
typedef struct {
	I2C_HandleTypeDef *hi2c;     /**< Pointer to the STM32 HAL I2C handle. */
	uint8_t  i2c_addr;           /**< 8-bit shifted I2C slave address. Refer to @ref INA_SLAVE_ADDRS. */
	float    r_shunt_ohm;        /**< Shunt resistor value in ohms (must be > 0). */
	float    max_current_a;      /**< Expected maximum current in amperes (must be > 0). */
	uint16_t bus_range;          /**< Bus voltage range. Refer to @ref INA_BUS_RANGE. */
	uint16_t pga_gain;           /**< Shunt voltage PGA gain. Refer to @ref INA_PGA_GAIN. */
	uint16_t bus_adc;            /**< Bus ADC resolution/averaging. Refer to @ref INA_BUS_ADC. */
	uint16_t shunt_adc;          /**< Shunt ADC resolution/averaging. Refer to @ref INA_SHUNT_ADC. */
	uint16_t mode;               /**< Sensor operating mode. Refer to @ref INA_OPERATING_MODE. */
} INA_Config_t;

/**
 * @brief Main handle structure for the INA219 sensor driver.
 */
typedef struct {
	INA_Config_t config;         /**< User-defined hardware and measurement configuration. */
	uint16_t     cal_value;      /**< Internal: computed 16-bit calibration register value. */
	float        current_lsb;    /**< Internal: current resolution in A/LSB. */
	float        power_lsb;      /**< Internal: power resolution in W/LSB. */
} INA_Handle_t;

/* ------------------------------------------------------------------ */


/* ------------------ Public APIs ------------------ */

/**
 * @brief Initialize the INA219.
 * @param[in,out] hina  pointer to INA handle structure with
 *                      desired configuration.
 * @retval HAL_OK       Initialization, configuration, and calibration succeeded.
 * @retval HAL_ERROR    Invalid configuration parameter, calibration overflow, or I2C error.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  I2C communication timed out.
 */
HAL_StatusTypeDef INA_Init(INA_Handle_t *hina);

/**
 * @brief Read the shunt voltage.
 * @param[in]  hina      pointer to INA handle structure.
 * @param[out] shunt_mv  shunt voltage drop over the shunt resistor in millivolts.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    NULL pointer passed or I2C communication error.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  I2C communication timed out.
 */
HAL_StatusTypeDef INA_ReadShuntVoltage_mV(const INA_Handle_t *hina, float *shunt_mv);

/**
 * @brief Read the bus voltage.
 * @param[in]  hina     pointer to INA handle structure.
 * @param[out] voltage  voltage read by INA.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    NULL pointer passed or I2C communication error.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  I2C communication timed out.
 */
HAL_StatusTypeDef INA_ReadBusVoltage_V(const INA_Handle_t *hina, float *voltage);

/**
 * @brief Read the current flowing through INA.
 * @param[in]  hina     pointer to INA handle structure.
 * @param[out] current  current read by INA.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    NULL pointer, device not calibrated, or I2C communication error.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  I2C communication timed out.
 */
HAL_StatusTypeDef INA_ReadCurrent_A(const INA_Handle_t *hina, float *current);

/**
 * @brief Read the power flowing through INA.
 * @param[in]  hina   pointer to INA handle structure.
 * @param[out] power  power read by INA.
 * @retval HAL_OK       Measurement read successfully.
 * @retval HAL_ERROR    NULL pointer, device not calibrated, or I2C communication error.
 * @retval HAL_BUSY     I2C peripheral is currently busy.
 * @retval HAL_TIMEOUT  I2C communication timed out.
 */
HAL_StatusTypeDef INA_ReadPower_W(const INA_Handle_t *hina, float *power);

/* ------------------------------------------------- */

#endif
