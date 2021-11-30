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
	  mfrc630_timer_set_reload(timer_for_timeout, 2000);  // 2000 ticks of 5 usec is 10 ms.
	  mfrc630_timer_set_value(timer_for_timeout, 2000);

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

	  //HAL_Delay_ms(155);

	  // all seems to be well...
	  uint8_t buffer_length = mfrc630_fifo_length();
	  //printf("mfrc630_fifo_length %d\n", buffer_length);
	  uint8_t rx_len = (buffer_length <= *dataSize) ? buffer_length : *dataSize;
	  mfrc630_read_fifo(data, rx_len);
	  *dataSize = rx_len;

	  return rx_len;
}

uint8_t mfrc630_rats()
{
	uint8_t  abtRats[] = { 0xe0, 0x80}; //8 means the PCD can processs a maxmum of 256 byte, tell the PICC do not exceed this number

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(abtRats, sizeof(abtRats),res, &resSize); // 05 78 80 40 00
	if(status>0)
	{
		if(status==5)
			printf("rats: %02X %02X %02X %02X %02X\n", res[0], res[1], res[2], res[3], res[4]);
		else
			printf("rats: %lu\n", resSize);
	}

	return status;
}

uint8_t mfrc630_halt()
{
//#define EM_mifs_PICC_HALT           0x50	// halt
//#define EM_mifs_PICC_DESELECT       0xCA	// DESELECT command code

	uint8_t  abtHalt[] = { 0x50, 0x00};

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(abtHalt, sizeof(abtHalt),res, &resSize); //
	if(status>0)
	{
		printf("halt: %lu\n", resSize);
	}

	return status;
}

/*
 *
 *
	DEP_REQ Frame
	(xx xx D4 06 PFB 55 AA 55 AA 55 AA)

	DEP_RES Frame
	(xx xx D5 07 PFB 00 01 02 …….)

 */
/*

+-----+-----+-----+-----+-----+-------------------------+-----+
| CLA | INS | P1  | P2  | Lc  | DATA                    | Le  |
+-----+-----+-----+-----+-----+-------------------------+-----+
| 00  | A4  | 04  | 00  | XX  | AID                     | 00  |
+-----+-----+-----+-----+-----+-------------------------+-----+

 */

uint8_t pcb = 0x02;

uint8_t mfrc630_APDU_select_app(uint8_t app[], uint32_t size)
{
	if(size!=7)
		return 0;

	uint8_t cmd[14] = {0x02, 0x00, 0xA4, 0x04, 0x00, 0x07};
	cmd[0] = pcb;
	if(pcb==0x02) pcb = 0x03;
	else pcb = 0x02;
	memcpy(&cmd[6], app, 7);
	cmd[13] = 0x00; //Le

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize);
	if(status>0)
	{
		if(status==2)
			printf("app: %02X %02X\n", res[0], res[1]);
		else
			printf("app: %lu\n", resSize);
	}

	return status;
}

uint8_t mfrc630_APDU_select_ppse(uint8_t ppse[], uint32_t size)
{
	if(size!=14)
			return 0;

	uint8_t cmd[21] = {0x02, 0x00, 0xA4, 0x04, 0x00, 0x0E};
	cmd[0] = pcb;
	if(pcb==0x02) pcb = 0x03;
	else pcb = 0x02;
	memcpy(&cmd[6], ppse, 14);
	cmd[20] = 0x00; //Le

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	//OK 90 00
	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize); //f2 08
	if(status>0)
	{
		if(status==2)
			printf("ppse: %02X %02X\n", res[0], res[1]);
		else
			printf("ppse: %lu\n", resSize);
	}

	return status;
}

uint8_t mfrc630_APDU_verify()
{
	uint8_t cmd[] = {0x02, 0x00, 0x20, 0x00, 0x01, 0x00};
	cmd[0] = pcb;
	if(pcb==0x02) pcb = 0x03;
	else pcb = 0x02;

	uint8_t res[256];
	uint32_t resSize = sizeof(res);
	memset(res, 0, resSize);

	//OK 90 00
	uint8_t status = mfrc630_transmit(cmd, sizeof(cmd), res, &resSize); //f2 08
	if(status>0)
	{
		if(status==2)
			printf("verify: %02X %02X\n", res[0], res[1]);
		else
			printf("verify: %lu\n", resSize);
	}

	return status;
}
