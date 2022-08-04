/*
 * mfrc.c
 *
 *  Created on: 17. 11. 2021
 *      Author: Development
 */

#include "mfrc.h"
#include "main.h"
#include "mfrc630.h"
#include "mfrc630_apdu.h"
#include "mfrc630_14443p4.h"
#include "delay.h"

#include <stdio.h>
#include <string.h>

static char readerData[17] = "";
static uint32_t	lastReading = 0;
static uint8_t  lastUid[10] = {0};
static uint8_t	lpcdIval = 0x23;
static uint8_t	lpcdQval = 0x25;

/*
 * Callback
 */
static MFRCReader_DATA_callback_t MfrcDataCallback = NULL;
void MFRCReaderDataCallbackRegister(MFRCReader_DATA_callback_t func)
{
	MfrcDataCallback = func;
}

const char* MFRCReader(uint8_t id)
{
	return readerData;
}

void MFRCReaderTask_Init(uint8_t lpcd)
{
	memset(readerData, 0, sizeof(readerData));
	memset(lastUid, 0, sizeof(lastUid));

	HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_SET);
	HAL_Delay_ms(1);
	HAL_GPIO_WritePin(RST_GPIO_Port, RST_Pin, GPIO_PIN_RESET);
	HAL_Delay_ms(1);

	mfrc630_AN1102_recommended_registers(MFRC630_PROTO_ISO14443A_106_MILLER_MANCHESTER);

	if(lpcd)
		mfrc630_LPCD(lpcdIval - 1, lpcdQval - 1);
}

void MFRCReader_LPCDmeas(uint8_t* i_val, uint8_t* q_val)
{
	mfrc630_LPCD_start_measurement();
	mfrc630_LPCD_stop_measurement(&lpcdIval, &lpcdQval);

	*i_val = lpcdIval;
	*q_val = lpcdQval;

	mfrc630_LPCD(lpcdIval - 1, lpcdQval - 1);
}

void MFRCReaderTask(uint8_t lpcd)
{
	if(lpcd)
	{
		memset(lastUid, 0, sizeof(lastUid));

		//check card removed
		uint32_t act = HAL_GetTick();
		if(lastReading + 400 > act)
		{
			lastReading = act;
			mfrc630_LPCD(lpcdIval - 1, lpcdQval - 1);
			return;
		}
		else
			lastReading = act;

		mfrc630_LPCD_wakup();
	}

	//check data
	uint16_t atqa = mfrc630_iso14443a_REQA();
	if (atqa != 0)
	{
		uint8_t sak;
		uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

		memset(uid, 0, sizeof(uid));

		// Select the card and discover its uid.
		uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);
		if(sak & 0x20) //bit 5 = support of ISO14443-4
		{
			mfrc630_rats();

#ifdef APDU_VISA
			uint8_t data[256];
			uint32_t dataSize = sizeof(data);

			uint8_t ppse[] = MFRC630_APDU_PPSE;
			mfrc630_APDU_select_ppse(ppse, sizeof(ppse), data, &dataSize);

			uint8_t app[] = MFRC630_APDU_APP_VISA;
			dataSize = sizeof(data);
			mfrc630_APDU_select_app(app, sizeof(app), data, &dataSize);
#else
			uint8_t data[16];
			uint32_t dataSize = sizeof(data);

			uint8_t app[] = {0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
			mfrc630_APDU_select_app(app, sizeof(app), data, &dataSize);
			if(dataSize==7)
			{
				memcpy(uid, &data[1], 6);
			}
#endif
		}

		if (uid_len > 0 && memcmp(uid, lastUid, uid_len) != 0)
		{
			memcpy(lastUid, uid, sizeof(lastUid));

			snprintf(readerData, sizeof(readerData), "%02X%02X%02X%02X%02X%02X%02X%02X",
					uid[3],
					uid[2],
					uid[1],
					uid[0],
					0,
					0,
					0,
					0);

			MAIN_DEBUG_TRACE(MFRC, ("Reader(NFC) data: %s; size: %d\n",
					readerData,
					uid_len));

			uint8_t uid_rev[4] = {uid[3],
								uid[2],
								uid[1],
								uid[0]};

			if(MfrcDataCallback)
				MfrcDataCallback(uid_rev, 4);
		}
		else if(uid_len > 0)
		{

		}
		else
		{
			memset(lastUid, 0, sizeof(lastUid));

			MAIN_DEBUG_TRACE(MFRC, ("Reader(NFC) no data\n"));
		}

		mfrc630_iso14443a_REQA();
	}
	else
	{
		memset(lastUid, 0, sizeof(lastUid));
	}

	if(lpcd)
		mfrc630_LPCD(lpcdIval - 1, lpcdQval - 1);
}

/*
 * MIUI - Settings/Connection&sharing/NFC On & Secure element position - HCE Wallet

		Sending REQA
		After waiting for answer
		rx_len: 02
		ATQA answer: 08 03
		UID input: 00 00 00 00 00 00 00 00 00 00

		Starting select

		CL: hd, coll loop: 01, kb 00 long:
		Setting rx align to: 00
		Send:02 long: 93 20
		irq0: 3C
		error: 00
		Got data, no collision, setting to: 20
		collision_pos: 20
		Fifo 05 long: 01 02 03 04 04
		uid_this_level kb 00 long:
		known_bits: 20
		exit collision loop: uid_this_level kb 32 long: 01 02 03 04 04 20 F9 FF FF 26
		send_req 07 long: 93 70 01 02 03 04 04
		SAK answer: 20
		UID of 4 bytes (SAK: 32): 01 02 03 04
		Could not authenticate :(
*/

/*
 * MIFARE Classic

		Sending REQA
		After waiting for answer
		rx_len: 02
		ATQA answer: 04 00
		UID input: 00 00 00 00 00 00 00 00 00 00

		Starting select

		CL: hd, coll loop: 01, kb 00 long:
		Setting rx align to: 00
		Send:02 long: 93 20
		irq0: 3C
		error: 00
		Got data, no collision, setting to: 20
		collision_pos: 20
		Fifo 05 long: A5 CD BB 79 AA
		uid_this_level kb 00 long:
		known_bits: 20
		exit collision loop: uid_this_level kb 32 long: A5 CD BB 79 AA 20 F9 FF FF 26
		send_req 07 long: 93 70 A5 CD BB 79 AA
		SAK answer: 08
		UID of 4 bytes (SAK: 8): A5 CD BB 79
		Authenticated!
		Read block 0x10 : A5 CD BB 79 AA 08 04 00 62 63 64 65 66 67 68 69
		Read block 0x10 : 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		Read block 0x10 : 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
		Read block 0x10 : 00 00 00 00 00 00 FF 07 80 69 FF FF FF FF FF FF

 */

// Hex print for blocks without printf.
static void print_block(uint8_t * block, uint8_t length){
    for (uint8_t i=0; i<length; i++){
        printf("%02X ", block[i]);
    }
    printf("\n");
}

// The example dump function adapted such that it prints with Serial.print.
void MFRCReader_Dump()
{
	uint32_t tick = HAL_GetTick();

	uint16_t atqa = mfrc630_iso14443a_REQA();
	if (atqa != 0)
	{  // Are there any cards that answered?
		uint8_t sak;
		uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

		// Select the card and discover its uid.
		uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);

		if (uid_len != 0)
		{  // did we get an UID?
			printf("[%06lu] MFRC uid %d bytes (SAK: 0x%02X, ATQA: 0x%04X): ", tick, uid_len, sak, atqa);
			print_block(uid, uid_len);

			if(sak & 0x20) //bit 5 = support of ISO14443-4
			{
				mfrc630_rats();

//#define APDU_VISA
#ifdef APDU_VISA
				uint8_t data[256];
				uint32_t dataSize;

				uint8_t ppse[] = MFRC630_APDU_PPSE;
				dataSize = sizeof(data);
				mfrc630_APDU_select_ppse(ppse, sizeof(ppse), data, &dataSize);

				uint8_t app[] = MFRC630_APDU_APP_VISA;
				dataSize = sizeof(data);
				mfrc630_APDU_select_app(app, sizeof(app), data, &dataSize);
				if(dataSize > 0)
				{
					printf("[%06lu] MFRC read 0x%02X:", tick, dataSize);
					print_block(data, dataSize);
				}
				else
				{
					printf("[%06lu] MFRC no read\n", tick);
				}
#else
				uint8_t data[16] = {0};
				uint32_t dataSize;

				uint8_t app[] = {0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
				uint8_t repeat = 2;

				do
				{
					dataSize = sizeof(data);
					mfrc630_APDU_select_app(app, sizeof(app), data, &dataSize);

					if(dataSize > 0)
					{
						printf("[%06lu] MFRC read 0x%02X:", tick, dataSize);
						print_block(data, dataSize);
						break;
					}

					repeat--;
				}while(repeat != 0);

				if(repeat == 0)
					printf("[%06lu] MFRC no read\n", tick);

#endif
				mfrc630_14443p4_deselect();
				mfrc630_iso14443a_WUPA();
			}
			else
			{
				// Use the manufacturer default key...
				uint8_t FFkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

				mfrc630_cmd_load_key(FFkey);  // load into the key buffer

				// Try to athenticate block 0.
				if (mfrc630_MF_auth(uid, MFRC630_MF_AUTH_KEY_A, 0))
				{
					// Attempt to read the first block.
					uint8_t readbuf[16] = {0};
					uint8_t len = mfrc630_MF_read_block(0, readbuf);

					if(len)
					{
						printf("[%06lu] MFRC read 0x%02X:", tick, len);
						print_block(readbuf, len);
					}
					else
						printf("[%06lu] MFRC no read\n", tick);

					mfrc630_MF_deauth();  // be sure to call this after an authentication!
				}
				else
				{
					printf("[%06lu] MFRC not authenticate\n", tick);
				}
			}
		}
		else
		{
			printf("[%06lu] MFRC no uid\n", tick);
		}
	}
}
