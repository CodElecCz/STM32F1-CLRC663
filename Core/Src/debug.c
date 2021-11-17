/*
 * debug.c
 *
 *  Created on: 26. 10. 2020
 *      Author: TURCAR
 */
#include "main.h"
#include "delay.h"

#if MAIN_DEBUG

#include "usbd_cdc_if.h"

#if MAIN_DEBUG_TO_UART6
extern UART_HandleTypeDef huart6;
#endif

/* Retargets the C library printf function to the USART. */
#include <stdio.h>
#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{
	return ch;
}
#ifdef __GNUC__
int _write(int file, char *ptr, int len)
{
#if MAIN_DEBUG_TO_UART6
	HAL_GPIO_WritePin(DE1_GPIO_Port, DE1_Pin, GPIO_PIN_SET);
	HAL_UART_Transmit(&huart6, (uint8_t *)ptr, len, 0xFFFF);
	HAL_GPIO_WritePin(DE1_GPIO_Port, DE1_Pin, GPIO_PIN_RESET);
#endif

#if MAIN_DEBUG_TO_USB
	uint32_t cnt = 0;
	uint8_t ret = CDC_Transmit_FS((uint8_t *)ptr, len);
	while(ret == USBD_BUSY)
	{
		HAL_Delay_us(100);
		ret = CDC_Transmit_FS((uint8_t *)ptr, len);
		if(++cnt>10)
			break;
	}
#endif

  int DataIdx;
  for (DataIdx= 0; DataIdx< len; DataIdx++)
  {
	__io_putchar(*ptr++);
  }

  return len;
}
#endif

#endif


