/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RST_Pin GPIO_PIN_0
#define RST_GPIO_Port GPIOA
#define BLED_Pin GPIO_PIN_1
#define BLED_GPIO_Port GPIOA
#define SDA_Pin GPIO_PIN_2
#define SDA_GPIO_Port GPIOA
#define GLED_Pin GPIO_PIN_0
#define GLED_GPIO_Port GPIOB
#define RLED_Pin GPIO_PIN_1
#define RLED_GPIO_Port GPIOB
#define IRQ_Pin GPIO_PIN_10
#define IRQ_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/*
 * DEBUG
 */
#ifdef DEBUG
#define MAIN_DEBUG 				1
#else //RELEASE
#define MAIN_DEBUG 				0
#define MAIN_RELEASE_TRACE		0
#endif

#if MAIN_DEBUG

//message has to include \n or call fflush()
#define MAIN_DEBUG_TRACE(module, message)  \
	if(MAIN_DEBUG_##module <= MAIN_DEBUG_LEVEL_TRACE) { printf message; }

//message has to include \n or call fflush()
#define MAIN_DEBUG_ERR(module, message)  \
	if(MAIN_DEBUG_##module <= MAIN_DEBUG_LEVEL_ERR) { printf message; }

//redirect to:
#define MAIN_DEBUG_TO_UART6			0
#define MAIN_DEBUG_TO_USB			1
//debug level
#define MAIN_DEBUG_LEVEL_ERR		2
#define MAIN_DEBUG_LEVEL_TRACE		1

#define MAIN_DEBUG_RFID				MAIN_DEBUG_LEVEL_ERR
#define MAIN_DEBUG_MFRC				MAIN_DEBUG_LEVEL_ERR

#else

#define MAIN_DEBUG_TRACE(module, message)
#define MAIN_DEBUG_ERR(module, message)

#endif


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
