#ifndef MFRC630_APDU_H_
#define MFRC630_APDU_H_

#include <stdint.h>
#include "mfrc630.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MFRC630_APDU_PPSE 				{0x32, 0x50, 0x41, 0x59, 0x2e, 0x53, 0x59, 0x53, 0x2e, 0x44, 0x44, 0x46, 0x30, 0x31}
#define MFRC630_APDU_APP_VISA 			{0xa0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10};
#define MFRC630_APDU_APP_MASTERCARD 	{0xa0, 0x00, 0x00, 0x00, 0x04, 0x10, 0x10};

uint8_t mfrc630_APDU_select_ppse(uint8_t ppse[], uint32_t ppseSize, uint8_t data[], uint32_t *dataSize);
uint8_t mfrc630_APDU_select_app(uint8_t app[], uint32_t appSize, uint8_t data[], uint32_t *dataSize);
uint8_t mfrc630_APDU_verify(uint8_t data[], uint32_t *dataSize);

#ifdef __cplusplus
}
#endif

#endif  // MFRC630_APDU_H_
