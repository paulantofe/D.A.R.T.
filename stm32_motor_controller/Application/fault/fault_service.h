#ifndef FAULT_FAULT_SERVICE_H_
#define FAULT_FAULT_SERVICE_H_

#include "stm32f4xx_hal.h"

/* ------------------ General Definitions ------------------ */

typedef enum {
	F_State_Normal = 0,
	F_State_Failsafe,
	F_State_Systemfault
} F_State_t;

/* ---------------------------------------------------------------- */


/* ------------------ Structures Definition ------------------ */

typedef struct {
	F_State_t fault_state;
} F_Handle_t;

/* ----------------------------------------------------------- */


/* ------------------ Public APIs ------------------ */

HAL_StatusTypeDef F_Init(F_Handle_t *hf);

HAL_StatusTypeDef F_EnterFailsafe(F_Handle_t *hf);

HAL_StatusTypeDef F_ExitFailsafe(F_Handle_t *hf);

void F_EnterSystemfault(F_Handle_t *hf);

/* ------------------------------------------------- */

#endif /* FAULT_FAULT_SERVICE_H_ */
