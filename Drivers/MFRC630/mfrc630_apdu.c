#include "mfrc630_apdu.h"
#include "mfrc630_def.h"
#include "delay.h"

#include <string.h>

uint8_t mfrc630_transmit(uint8_t cmd[], uint32_t cmdSize, uint8_t data[], uint32_t* dataSize)
{
	mfrc630_flush_fifo();

	  mfrc630_write_reg(MFRC630_REG_TXCRCPRESET, MFRC630_RECOM_14443A_CRC | MFRC630_CRC_ON);
	  mfrc630_write_reg(MFRC630_REG_RXCRCCON, MFRC630_RECOM_14443A_CRC | MFRC630_CRC_ON);

	  // configure a timeout timer.
	  uint8_t timer_for_timeout = 0;  // should match the enabled interupt.

	  // enable the global IRQ for idle, errors and timer.
	  mfrc630_write_reg(MFRC630_REG_IRQ0EN, MFRC630_IRQ0EN_IDLE_IRQEN | MFRC630_IRQ0EN_ERR_IRQEN);
	  mfrc630_write_reg(MFRC630_REG_IRQ1EN, MFRC630_IRQ1EN_TIMER0_IRQEN);


	  // Set timer to 221 kHz clock, start at the end of Tx.
	  mfrc630_timer_set_control(timer_for_timeout, MFRC630_TCONTROL_CLK_211KHZ | MFRC630_TCONTROL_START_TX_END);
	  // Frame waiting time: FWT = (256 x 16/fc) x 2 FWI
	  // FWI defaults to four... so that would mean wait for a maximum of ~ 5ms
	  mfrc630_timer_set_reload(timer_for_timeout, 20000);  // 20000 ticks of 5 usec is 100 ms.
	  mfrc630_timer_set_value(timer_for_timeout, 20000);

	  uint8_t irq1_value = 0;
	  uint8_t irq0_value = 0;

	  mfrc630_clear_irq0();  // clear irq0
	  mfrc630_clear_irq1();  // clear irq1

	  // Go into send, then straight after in receive.
	  //uint8_t send_req[2] = {MFRC630_MF_CMD_READ, block_address};
	  mfrc630_cmd_transceive(cmd, cmdSize);

	  // block until we are done
	  while (!(irq1_value & (1 << timer_for_timeout))) {
	    irq1_value = mfrc630_irq1();
	    if (irq1_value & MFRC630_IRQ1_GLOBAL_IRQ) {
	      break;  // stop polling irq1 and quit the timeout loop.
	    }
	  }
	  mfrc630_cmd_idle();

	  if (irq1_value & (1 << timer_for_timeout)) {
	    // this indicates a timeout
		  printf("timeout\n");
	    return 0;
	  }

	  irq0_value = mfrc630_irq0();
	  if (irq0_value & MFRC630_IRQ0_ERR_IRQ) {
	    // some error
		uint8_t error = mfrc630_read_reg(MFRC630_REG_ERROR);
		printf("error: %02x\n", error);
	    return 0;
	  }

	  // all seems to be well...
	  uint8_t buffer_length = mfrc630_fifo_length();
	  //printf("mfrc630_fifo_length %d\n", buffer_length);
	  uint8_t rx_len = (buffer_length <= *dataSize) ? buffer_length : *dataSize;
	  mfrc630_read_fifo(data, rx_len);
	  *dataSize = rx_len;

	  return rx_len;
}

static void print_block(uint8_t * block, uint8_t length){
    for (uint8_t i=0; i<length; i++){
        printf("%02X ", block[i]);
    }
    printf("\n");
}

uint8_t mfrc630_rats()
{
	uint8_t  abtRats[] = {0xe0, 0x80}; //8 means the PCD can processs a maxmum of 256 byte, tell the PICC do not exceed this number

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(abtRats, sizeof(abtRats), res, &resSize);
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
	uint8_t  abtHalt[] = {0x50, 0x00};

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(abtHalt, sizeof(abtHalt),res, &resSize); //
	if(status>0)
	{
		printf("HALT (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}

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

uint8_t mfrc630_APDU_select_app(uint8_t app[], uint32_t size)
{
	if(size!=7)
		return 0;

	uint8_t cmd[14] = {0x00, 0x00, 0xA4, 0x04, 0x00, 0x07};
	cmd[0] = get_pcb();
	memcpy(&cmd[6], app, 7);
	cmd[13] = 0x00; //Le

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize);
	if(status>0)
		{
			printf("APDU APP (%lub): ", resSize);
			print_block(res, resSize);
		}

	return status;
}

uint8_t mfrc630_APDU_select_ppse(uint8_t ppse[], uint32_t size)
{
	if(size!=14)
		return 0;

	uint8_t cmd[21] = {0x00, 0x00, 0xA4, 0x04, 0x00, 0x0E};
	cmd[0] = get_pcb();
	memcpy(&cmd[6], ppse, 14);
	cmd[20] = 0x00; //Le

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	//OK 90 00
	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize); //f2 08
	if(status>0)
	{
		printf("APDU PPSE (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}

uint8_t mfrc630_APDU_verify()
{
	uint8_t cmd[] = {0x00, 0x00, 0x20, 0x00, 0x01, 0x00};
	cmd[0] = get_pcb();

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	//OK 90 00
	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize);
	if(status>0)
	{
		printf("APDU VERIFY (%lub): ", resSize);
		print_block(res, resSize);
	}

	return status;
}
