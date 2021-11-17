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

void USB_Transmit(const char *ptr, int len)
{
	uint32_t cnt = 0;
	uint8_t ret = CDC_Transmit_FS((uint8_t *)ptr, len);
	while(ret == USBD_BUSY)
	{
		HAL_Delay_us(50);
		ret = CDC_Transmit_FS((uint8_t *)ptr, len);
		if(++cnt>10)
			break;
	}
}

// The example dump function adapted such that it prints with Serial.print.
void mfrc630_MF_example_dump_arduino() {
  uint16_t atqa = mfrc630_iso14443a_REQA();
  if (atqa != 0) {  // Are there any cards that answered?
    uint8_t sak;
    uint8_t uid[10] = {0};  // uids are maximum of 10 bytes long.

    // Select the card and discover its uid.
    uint8_t uid_len = mfrc630_iso14443a_select(uid, &sak);

    if (uid_len != 0) {  // did we get an UID?
      Serial.print("UID of ");
      Serial.print(uid_len);
      Serial.print(" bytes (SAK: ");
      Serial.print(sak);
      Serial.print("): ");
      print_block(uid, uid_len);
      Serial.print("\n");

      // Use the manufacturer default key...
      uint8_t FFkey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

      mfrc630_cmd_load_key(FFkey);  // load into the key buffer

      // Try to athenticate block 0.
      if (mfrc630_MF_auth(uid, MFRC630_MF_AUTH_KEY_A, 0)) {
        Serial.println("Yay! We are authenticated!");

        // Attempt to read the first 4 blocks.
        uint8_t readbuf[16] = {0};
        uint8_t len;
        for (uint8_t b=0; b < 4 ; b++) {
          len = mfrc630_MF_read_block(b, readbuf);
          Serial.print("Read block 0x");
          print_block(&len,1);
          Serial.print(": ");
          print_block(readbuf, len);
          Serial.println();
        }
        mfrc630_MF_deauth();  // be sure to call this after an authentication!
      } else {
        Serial.print("Could not authenticate :(\n");
      }
    } else {
      Serial.print("Could not determine UID, perhaps some cards don't play");
      Serial.print(" well with the other cards? Or too many collisions?\n");
    }
  } else {
    Serial.print("No answer to REQA, no cards?\n");
  }
}
