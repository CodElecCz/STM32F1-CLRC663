/*
 * mfrc.c
 *
 *  Created on: 17. 11. 2021
 *      Author: Development
 */

#include "mfrc.h"
#include "main.h"
#include "mfrc630.h"
#include "usbd_cdc_if.h"
#include "delay.h"

#include <stdio.h>

// Hex print for blocks without printf.
static void print_block(uint8_t * block,uint8_t length){
    for (uint8_t i=0; i<length; i++){
        printf("%02X ", block[i]);
    }
}

// The example dump function adapted such that it prints with Serial.print.
void mfrc_dump() {
  uint16_t atqa = mfrc630_iso14443a_REQA();
  if (atqa != 0) {  // Are there any cards that answered?
    uint8_t sak;
    uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

    // Select the card and discover its uid.
    uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);

    if (uid_len != 0) {  // did we get an UID?
      printf("UID of %d bytes (SAK: %d): ", uid_len, sak);
      print_block(uid, uid_len);
      printf("\n");

      // Use the manufacturer default key...
      uint8_t FFkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

      mfrc630_cmd_load_key(FFkey);  // load into the key buffer

      // Try to athenticate block 0.
      if (mfrc630_MF_auth(uid, MFRC630_MF_AUTH_KEY_A, 0)) {
    	  printf("Authenticated!\n");

        // Attempt to read the first 4 blocks.
        uint8_t readbuf[16] = {0};
        uint8_t len;
        for (uint8_t b=0; b < 4 ; b++) {
          len = mfrc630_MF_read_block(b, readbuf);
          printf("Read block 0x");
          print_block(&len,1);
          printf(": ");
          print_block(readbuf, len);
          printf("\n");
        }
        mfrc630_MF_deauth();  // be sure to call this after an authentication!
      } else {
    	  printf("Could not authenticate :(\n");
      }
    } else {
    	printf("Could not determine UID, perhaps some cards don't play");
    	printf(" well with the other cards? Or too many collisions?\n");
    }
  } else {
	  printf("No answer to REQA, no cards?\n");
  }
}
