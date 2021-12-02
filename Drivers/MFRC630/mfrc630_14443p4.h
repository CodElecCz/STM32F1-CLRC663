/*
 * mfrc630_14443p4.h
 *
 *  Created on: 2. 12. 2021
 *      Author: Development
 */

#ifndef MFRC630_MFRC630_14443P4_H_
#define MFRC630_MFRC630_14443P4_H_

#include <stdint.h>
#include "mfrc630.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t mfrc630_rats();
uint8_t mfrc630_halt();

uint8_t mfrc630_14443p4_transfer(uint8_t cmd[], uint32_t cmdSize, uint8_t data[], uint32_t* dataSize);

#ifdef __cplusplus
}
#endif

#endif /* MFRC630_MFRC630_14443P4_H_ */
