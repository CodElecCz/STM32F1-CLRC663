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

static uint8_t get_pcb()
{
	uint8_t _pcb = pcb;

	if(pcb==0x02)
		pcb = 0x03;
	else
		pcb = 0x02;

	return _pcb;
}

static void print_block(uint8_t * block, uint8_t length){
    for (uint8_t i=0; i<length; i++){
        printf("%02X ", block[i]);
    }
    printf("\n");
}

uint8_t mfrc630_rats()
{
	uint8_t  req[] = {0xe0, 0x80}; //8 means the PCD can processs a maxmum of 256 byte, tell the PICC do not exceed this number

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transfer(req, sizeof(req), res, &resSize);
	//MI: 		05 78 80 40 00
	//SAMSUNG: 	05 78 80 70 00
	//VISA:		08 78 80 70 00 00 00 00
	if(status>0)
	{
		printf("RATS (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}

uint8_t mfrc630_halt()
{
	uint8_t  req[] = {0x50, 0x00};

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transfer(req, sizeof(req), res, &resSize);
	if(status>0)
	{
		printf("HALT (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}


uint8_t mfrc630_14443p4_transfer(uint8_t cmd[], uint32_t cmdSize, uint8_t data[], uint32_t* dataSize)
{
	uint8_t req[cmdSize+1];
	req[0] = get_pcb();
	memcpy(&req[1], cmd, cmdSize);

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transfer(req, sizeof(req),res, &resSize);
	if(status>0)
	{
		printf("APDU (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}
