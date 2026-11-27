#ifndef FAULT_FAULT_SERVICE_H_
#define FAULT_FAULT_SERVICE_H_

#include "stm32f4xx_hal.h"

/* ------------------ Public APIs ------------------ */

HAL_StatusTypeDef F_Init(void);

HAL_StatusTypeDef F_EnterFailsafe(void);

HAL_StatusTypeDef F_ExitFailsafe(void);

void F_EnterSystemfault(void);

/* ------------------------------------------------- */

#endif /* FAULT_FAULT_SERVICE_H_ */
