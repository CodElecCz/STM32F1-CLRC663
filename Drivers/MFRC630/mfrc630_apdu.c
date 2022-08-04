#include "mfrc630_apdu.h"
#include "mfrc630_def.h"
#include "mfrc630_14443p4.h"
#include "delay.h"

#include <string.h>

/*

APDU

	+-----+-----+-----+-----+-----+-------------------------+-----+
	| CLA | INS | P1  | P2  | Lc  | DATA                    | Le  |
	+-----+-----+-----+-----+-----+-------------------------+-----+
	| 00  | A4  | 04  | 00  | XX  | AID                     | 00  |
	+-----+-----+-----+-----+-----+-------------------------+-----+

VISA
	RATS (8b): 08 78 80 70 00 00 00 00
	APDU PPSE (63b): 02 6F 3A 84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 28 BF 0C 25 61 23 4F 07 A0 00 00 00 03 10 10 50 0A 56 69 73 61 20 44 65 62 69 74 87 01 01 9F 0A 08 00 01 05 01 00 00 00 00 90 00
	APDU APP (106b): 03 6F 65 84 07 A0 00 00 00 03 10 10 A5 5A 50 0A 56 69 73 61 20 44 65 62 69 74 87 01 01 5F 2D 02 65 6E 9F 11 01 01 9F 12 07 52 65 76 6F 6C 75 74 9F 38 18 9F 66 04 9F 02 06 9F 03 06 9F 1A 02 95 05 5F 2A 02 9A 03 9C 01 9F 37 04 BF 0C 1A 9F 5A 05 31 08 26 08 26 9F 0A 08 00 01 05 01 00 00 00 00 BF 63 04 DF 20 01 80 90 00

MASTERCARD
	RATS (20b): 14 78 80 70 02 41 43 4F 53 20 44 49 61 6E 61 32 20 4C 63 36
	APDU PPSE (51b): 02 6F 2E 84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 1C BF 0C 19 61 17 4F 07 A0 00 00 00 04 10 10 87 01 01 9F 0A 08 00 01 05 02 00 00 00 00 90 00
	APDU APP (55b): 03 6F 32 84 07 A0 00 00 00 04 10 10 A5 27 50 0A 4D 61 73 74 65 72 63 61 72 64 87 01 01 BF 0C 15 9F 6E 07 02 03 00 00 30 30 00 9F 0A 08 00 01 05 02 00 00 00 00 90 00

HCE
	RATS (5b): 05 78 80 40 00
	APDU PPSE (2b): F2 08 			???

 */

uint8_t mfrc630_APDU_select_app(uint8_t app[], uint32_t appSize, uint8_t data[], uint32_t *dataSize)
{
	if(appSize != 7)
		return 0;

	uint8_t cmd[13] = {0x00, 0xA4, 0x04, 0x00, 0x07};
	memcpy(&cmd[5], app, appSize);
	cmd[12] = 0x00; //Le

	return mfrc630_14443p4_transfer(cmd, sizeof(cmd), data, dataSize);
}

uint8_t mfrc630_APDU_select_ppse(uint8_t ppse[], uint32_t ppseSize, uint8_t data[], uint32_t *dataSize)
{
	if(ppseSize != 14)
		return 0;

	uint8_t cmd[20] = {0x00, 0xA4, 0x04, 0x00, 0x0E};
	memcpy(&cmd[5], ppse, ppseSize);
	cmd[19] = 0x00; //Le

	return mfrc630_14443p4_transfer(cmd, sizeof(cmd), data, dataSize); //f2 08
}

uint8_t mfrc630_APDU_verify(uint8_t data[], uint32_t *dataSize)
{
	uint8_t cmd[] = {0x00, 0x20, 0x00, 0x01, 0x00};

	return mfrc630_14443p4_transfer(cmd, sizeof(cmd), data, dataSize);
}
