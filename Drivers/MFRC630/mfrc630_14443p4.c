/*
 * mfrc630_14443p4.c
 *
 *  Created on: 2. 12. 2021
 *      Author: Development
 */

#include "mfrc630_14443p4.h"
#include "mfrc630_def.h"
#include "delay.h"

#include <string.h>

static uint8_t pcb = 0x02;

static const uint16_t bI14443p4_FsTable[13] = {16, 24, 32,
											   40, 48, 64,
											   96, 128, 256,
											   512, 1024,
											   2048, 4096};

static void print_block(uint8_t * block, uint8_t length){
    for (uint8_t i=0; i<length; i++){
    	MFRC630_PRINTF("%02X ", block[i]);
    }
    MFRC630_PRINTF("\n");
}

uint8_t mfrc630_rats()
{
	uint8_t  req[] = {0xe0, 0x80}; //8 means the PCD can processs a maxmum of 256 byte, tell the PICC do not exceed this number
	uint32_t reqSize = sizeof(req);

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	MFRC630_PRINTF("ATS (%lub) > ", reqSize);
	print_block(req, reqSize);

	uint8_t status = mfrc630_transfer(req, reqSize, res, &resSize);
	//MI: 		05 78 80 40 00
	//SAMSUNG: 	05 78 80 70 00
	//VISA:		08 78 80 70 00 00 00 00
	if(status>0)
	{
		MFRC630_PRINTF("ATS (%lub) < ", resSize);
		print_block(res, resSize);
	}

	return status;
}

//I-Block
static uint8_t get_pcb()
{
	uint8_t _pcb = pcb;

	if(pcb==0x02)
		pcb = 0x03;
	else
		pcb = 0x02;

	return _pcb;
}

#define PHPAL_I14443P4_SW_IS_BLOCKNR_EQUAL(bPcb)                        \
    (                                                                   \
        ((((bPcb) & PHPAL_I14443P4_SW_PCB_BLOCKNR) ^ pDataParams->bPcbBlockNum) == 0U) \
        ? 1u : 0u                                                       \
    )

#define PHPAL_I14443P4_SW_IS_ACK(bPcb)                                  \
    (                                                                   \
    (((bPcb) & PHPAL_I14443P4_SW_PCB_NAK) == PHPAL_I14443P4_SW_PCB_ACK) \
    ? 1u : 0u                                                             \
    )

#define PHPAL_I14443P4_SW_IS_CHAINING(bPcb)                                         \
    (                                                                               \
    (((bPcb) & PHPAL_I14443P4_SW_PCB_CHAINING) == PHPAL_I14443P4_SW_PCB_CHAINING)   \
    ? 1u : 0u                                                                         \
    )

#define PHPAL_I14443P4_SW_IS_WTX(bPcb)                                  \
    (                                                                   \
    (((bPcb) & PHPAL_I14443P4_SW_PCB_WTX) == PHPAL_I14443P4_SW_PCB_WTX) \
    ? 1u : 0u                                                             \
    )

#define PHPAL_I14443P4_SW_IS_DESELECT(bPcb)                                     \
    (                                                                           \
    (((bPcb) & PHPAL_I14443P4_SW_PCB_WTX) == PHPAL_I14443P4_SW_PCB_DESELECT)    \
    ? 1u : 0u                                                                     \
    )

#define PHPAL_I14443P4_SW_IS_I_BLOCK(bPcb)                                  \
    (                                                                       \
    (((bPcb) & PHPAL_I14443P4_SW_BLOCK_MASK) == PHPAL_I14443P4_SW_I_BLOCK)  \
    ? 1u : 0u                                                                 \
    )

#define PHPAL_I14443P4_SW_IS_R_BLOCK(bPcb)                                  \
    (                                                                       \
    (((bPcb) & PHPAL_I14443P4_SW_BLOCK_MASK) == PHPAL_I14443P4_SW_R_BLOCK)  \
    ? 1u : 0u                                                                 \
    )

#define PHPAL_I14443P4_SW_IS_S_BLOCK(bPcb)                                  \
    (                                                                       \
    (((bPcb) & PHPAL_I14443P4_SW_BLOCK_MASK) == PHPAL_I14443P4_SW_S_BLOCK)  \
    ? 1u : 0u                                                                 \
    )

/**
 *
 * Frame Format for ISO/IEC 14443-4
 * ================================
 *
 * The frame format ISO 14443-4 specifications for block formats.
 * This is the format used by the example firmware, and seen in Figure 3.
 *  - PCB – Protocol Control Byte, this byte is used to transfer format information about each PDU block.
 *  - CID – Card Identifier field, this byte is used to identify specific tags. It contains a 4 bit CID value as well
 *          as information on the signal strength between the reader and the tag.
 *  - NAD – Node Address field, the example firmware does not support the use of NAD.
 *  - INF – Information field
 *  - EDC – CRC of the transmitted block, which is the CRC defined in ISO/IEC 14443-3
 *
 *  |-----|-----|-----|----------------|-----|
 *  | PCB | CID | NAD |      INF       | EDC |
 *  |-----|-----|-----|----------------|-----|
 *
 */

uint8_t mfrc630_14443p4_transfer(uint8_t cmd[], uint32_t cmdSize, uint8_t data[], uint32_t* dataSize)
{
	uint8_t req[cmdSize+1];
	req[0] = get_pcb();
	memcpy(&req[1], cmd, cmdSize);
	uint32_t reqSize = sizeof(req);

	uint8_t res[128];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint32_t dataSizeAlloc = *dataSize;
	*dataSize = 0;

	uint8_t status = 0;

	MFRC630_PRINTF("14443P4 APDU (%lub) > ", reqSize);
	print_block(req, reqSize);

	uint32_t retries = 6;

	do
	{
		uint32_t resSize = sizeof(res);
		status = mfrc630_transfer(req, reqSize, res, &resSize);
		if(status>0)
		{
			MFRC630_PRINTF("14443P4 (%lub) < ", resSize);
			print_block(res, resSize);

			/* NXP phpalI14443p4_Sw.c */
			/* I-Block handling */
			if (0u != (PHPAL_I14443P4_SW_IS_I_BLOCK(res[PHPAL_I14443P4_SW_PCB_POS])))
			{
				//OK
				size_t size = (resSize < dataSizeAlloc)? resSize : dataSizeAlloc;
				memcpy(data, res, size);
				*dataSize = size;
				break;
			}
			/* R(ACK) handling */
			else if ((PHPAL_I14443P4_SW_IS_R_BLOCK(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U) && (PHPAL_I14443P4_SW_IS_ACK(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U))
			{
				break;
			}
			/* S(WTX) handling */
			else if ((PHPAL_I14443P4_SW_IS_S_BLOCK(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U) && (PHPAL_I14443P4_SW_IS_WTX(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U))
			{
				/* Retrieve WTXM */
				uint8_t bWtxm = res[resSize-1u];

				/* Ignore and clear the Power Level Indication */
				bWtxm &= 0x3FU;

				/* Generate S(WTX) frame */

				/* S-Block PCB */
				req[PHPAL_I14443P4_SW_PCB_POS]  = PHPAL_I14443P4_SW_S_BLOCK | PHPAL_I14443P4_SW_S_BLOCK_RFU_BITS;
				req[PHPAL_I14443P4_SW_PCB_POS] |= PHPAL_I14443P4_SW_PCB_WTX;
				req[PHPAL_I14443P4_SW_PCB_POS+1] = bWtxm;
				reqSize = 2;

				MFRC630_PRINTF("14443P4 S(WTX) (%lub) > ", reqSize);
				print_block(req, reqSize);

				//TODO: wait?
			}
			/* S(DESELECT) handling */
			else if ((PHPAL_I14443P4_SW_IS_S_BLOCK(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U) && (PHPAL_I14443P4_SW_IS_DESELECT(res[PHPAL_I14443P4_SW_PCB_POS]) > 0U))
			{
				/* S-Block PCB */
				req[PHPAL_I14443P4_SW_PCB_POS]  = PHPAL_I14443P4_SW_S_BLOCK | PHPAL_I14443P4_SW_S_BLOCK_RFU_BITS;
				reqSize = 1;

				MFRC630_PRINTF("14443P4 S(DESELECT) (%lub) > ", reqSize);
				print_block(req, reqSize);
			}
			/* We received an invalid block */
			else
			{
				break;
			}
		}
		else
			break;

		--retries;
	}while(retries != 0);

	return status;
}

uint8_t mfrc630_14443p4_deselect()
{
	uint8_t status = 0;

	uint8_t req[1];
	uint32_t reqSize;

	/* S-Block PCB */
	req[PHPAL_I14443P4_SW_PCB_POS]  = PHPAL_I14443P4_SW_S_BLOCK | PHPAL_I14443P4_SW_S_BLOCK_RFU_BITS;
	req[PHPAL_I14443P4_SW_PCB_POS] |= PHPAL_I14443P4_SW_PCB_DESELECT;
	reqSize = 1;

	MFRC630_PRINTF("14443P4 S(DESELECT) (%lub) > ", reqSize);
	print_block(req, reqSize);

	uint8_t res[8];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	status = mfrc630_transfer(req, reqSize, res, &resSize);
	if(status>0)
	{
		MFRC630_PRINTF("14443P4 (%lub) < ", resSize);
		print_block(res, resSize);
	}

	return status;
}
