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
#include "usbd_cdc_if.h"
#include "delay.h"

#include <stdio.h>

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

#define EOL 	"\r\n"
#define response(data)	USB_Transmit((uint8_t*)data, strlen(data))

void USB_Transmit(uint8_t *ptr, int len)
{
	uint32_t cnt = 0;
	uint8_t ret = CDC_Transmit_FS(ptr, len);
	while(ret == USBD_BUSY)
	{
		HAL_Delay_us(50);
		ret = CDC_Transmit_FS(ptr, len);
		if(++cnt>10)
			break;
	}
#if REDIRECT_TO_UART
	UART_Transmit(ptr, len);
#endif
}

// Hex print for blocks without printf.
static void print_block(uint8_t * block, uint8_t length){
    for (uint8_t i=0; i<length; i++){
        printf("%02X ", block[i]);
    }
    printf("\n");
}

static uint32_t cnt = 0;

// The example dump function adapted such that it prints with Serial.print.
void mfrc_dump() {
  uint16_t atqa = mfrc630_iso14443a_REQA();
  if (atqa != 0) {  // Are there any cards that answered?
    uint8_t sak;
    uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

    // Select the card and discover its uid.
    uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);

    if (uid_len != 0) {  // did we get an UID?
#if DEBUG
      printf("UID of %d bytes (SAK: 0x%02X, ATQA: 0x%04X): ", uid_len, sak, atqa);
      print_block(uid, uid_len);

      if(sak & 0x20) //bit 5 = support of ISO14443-4
      {
		  mfrc630_rats();

		  uint8_t ppse[] = MFRC630_APDU_PPSE;
		  mfrc630_APDU_select_ppse(ppse, sizeof(ppse));

		  uint8_t app[] = MFRC630_APDU_APP_VISA;
		  mfrc630_APDU_select_app(app, sizeof(app));
      }
      else
      {
		  // Use the manufacturer default key...
		  uint8_t FFkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

		  mfrc630_cmd_load_key(FFkey);  // load into the key buffer

		  // Try to athenticate block 0.
		  if (mfrc630_MF_auth(uid, MFRC630_MF_AUTH_KEY_A, 0))
		  {
			  printf("Authenticated!\n");

			// Attempt to read the first 4 blocks.
			uint8_t readbuf[16] = {0};
			uint8_t len;
			for (uint8_t b=0; b < 4 ; b++)
			{
			  len = mfrc630_MF_read_block(b, readbuf);
			  printf("Read block 0x%02X: ", len);
			  print_block(readbuf, len);
			}
			mfrc630_MF_deauth();  // be sure to call this after an authentication!
		  }
		  else
		  {
			  printf("Could not authenticate :(\n");
		  }
      }
#else
      char buffer[256];
      char* pbuffer = buffer;
      pbuffer += snprintf(buffer, sizeof(buffer), "[%08lu] UID of %d bytes (SAK: 0x%02X, ATQA: 0x%04X): ", HAL_GetTick(), uid_len, sak, atqa);
      for (uint8_t i=0; i<uid_len; i++){
    	  pbuffer += sprintf(pbuffer, "%02X ", uid[i]);
          }
      pbuffer += sprintf(pbuffer, EOL);
      response(buffer);
#endif

      //dummy call
      mfrc630_iso14443a_REQA();
    }
    else
    {
    	printf("Could not determine UID, perhaps some cards don't play");
    	printf(" well with the other cards? Or too many collisions?\n");
    }
  }
  else
  {
#if DEBUG
	  printf("No answer to REQA, no cards?\n");
#else
	  char buffer[256];
	  snprintf(buffer, sizeof(buffer), "[%08lu] no read"EOL, HAL_GetTick());
	  response(buffer);
#endif
  }
}
