/*
 * delay.h
 *
 *  Created on: Nov 10, 2021
 *      Author: radomir.turca@codelec.cz
 */

#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void HAL_Delay_us(uint32_t us);
void HAL_Delay_ms(uint32_t ms);	//max 655ms

#ifdef __cplusplus
}
#endif

#endif /* INC_DELAY_H_ */
