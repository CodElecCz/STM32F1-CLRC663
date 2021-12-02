#ifndef MFRC630_APDU_H_
#define MFRC630_APDU_H_

#include <stdint.h>
#include "mfrc630.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t mfrc630_APDU_select_ppse(uint8_t ppse[], uint32_t size);
uint8_t mfrc630_APDU_select_app(uint8_t app[], uint32_t size);
uint8_t mfrc630_APDU_verify();

#ifdef __cplusplus
}
#endif

#endif  // MFRC630_APDU_H_
