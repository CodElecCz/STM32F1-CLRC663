/*
 * delay.c
 *
 *  Created on: Nov 10, 2021
 *      Author: radomir.turca@codelec.cz
 */

#include "delay.h"
#include "main.h"

#include <stdint.h>

extern TIM_HandleTypeDef htim1;

//handle 16 bit counter resolution
static void delay_us(uint16_t us)
{
	if(htim1.Instance != NULL)
	{
		__HAL_TIM_SET_COUNTER(&htim1, 0);  // set the counter value a 0
		while (__HAL_TIM_GET_COUNTER(&htim1) < us);  // wait for the counter to reach the us
	}
}

void HAL_Delay_us(uint32_t us)
{
	uint32_t us_act = us/10;
	while(us_act>UINT16_MAX)
	{
		delay_us(UINT16_MAX);
		us_act -= UINT16_MAX;
	}
	delay_us(us_act);
}

void HAL_Delay_ms(uint32_t ms)
{
	HAL_Delay_us(ms*1000);
}
