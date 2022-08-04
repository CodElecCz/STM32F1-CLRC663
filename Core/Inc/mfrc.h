/*
 * mfrc.h
 *
 *  Created on: 17. 11. 2021
 *      Author: Development
 */

#ifndef INC_MFRC_H_
#define INC_MFRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void MFRCReaderTask_Init(uint8_t lpcd);
void MFRCReaderTask(uint8_t lpcd);
const char* MFRCReader(uint8_t id);

void MFRCReader_Dump();
void MFRCReader_LPCDmeas(uint8_t* i_val, uint8_t* q_val);

/*
 * Data Callback
 */
typedef void (*MFRCReader_DATA_callback_t)(const uint8_t*, uint32_t);
void MFRCReaderDataCallbackRegister(MFRCReader_DATA_callback_t func);

#ifdef __cplusplus
}
#endif

#endif /* INC_MFRC_H_ */
