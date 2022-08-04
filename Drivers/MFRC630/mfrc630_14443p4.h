/*
 * mfrc630_14443p4.h
 *
 *  Created on: 2. 12. 2021
 *      Author: Development
 */

#ifndef MFRC630_MFRC630_14443P4_H_
#define MFRC630_MFRC630_14443P4_H_

#include <stdint.h>
#include "mfrc630.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t mfrc630_rats();
uint8_t mfrc630_14443p4_transfer(uint8_t cmd[], uint32_t cmdSize, uint8_t data[], uint32_t* dataSize);
uint8_t mfrc630_14443p4_deselect();

/** ISO 14443-4 Block Mask */
#define PHPAL_I14443P4_SW_BLOCK_MASK            0xC0U

/** ISO 14443-4 I-Block */
#define PHPAL_I14443P4_SW_I_BLOCK               0x00U
/** ISO 14443-4 I-Block RFU mask */
#define PHPAL_I14443P4_SW_I_BLOCK_RFU_MASK      0x22U
/** ISO 14443-4 I-Block RFU bits */
#define PHPAL_I14443P4_SW_I_BLOCK_RFU_BITS      0x02U

/** ISO 14443-4 R-Block */
#define PHPAL_I14443P4_SW_R_BLOCK               0x80U
/** ISO 14443-4 R-Block RFU mask */
#define PHPAL_I14443P4_SW_R_BLOCK_RFU_MASK      0x26U
/** ISO 14443-4 R-Block RFU bits */
#define PHPAL_I14443P4_SW_R_BLOCK_RFU_BITS      0x22U

/** ISO 14443-4 S-Block */
#define PHPAL_I14443P4_SW_S_BLOCK               0xC0U
/** ISO 14443-4 S-Block RFU mask */
#define PHPAL_I14443P4_SW_S_BLOCK_RFU_MASK      0x07U
/** ISO 14443-4 S-Block RFU bits */
#define PHPAL_I14443P4_SW_S_BLOCK_RFU_BITS      0x02U

/** Byte position of PCB byte */
#define PHPAL_I14443P4_SW_PCB_POS               0x00U

/** Blocknumber Bit within PCB */
#define PHPAL_I14443P4_SW_PCB_BLOCKNR           0x01U

/** Bit within PCB which signals presence of NAD byte */
#define PHPAL_I14443P4_SW_PCB_NAD_FOLLOWING     0x04U

/** Bit within PCB which signals presence of CID byte */
#define PHPAL_I14443P4_SW_PCB_CID_FOLLOWING     0x08U

/** Bit within PCB which signals chaining */
#define PHPAL_I14443P4_SW_PCB_CHAINING          0x10U

/** Bit within PCB which signals ACK */
#define PHPAL_I14443P4_SW_PCB_ACK               0x00U

/** Bit within PCB which signals NAK */
#define PHPAL_I14443P4_SW_PCB_NAK               0x10U

/** Bits within PCB which signal a DESELECT command */
#define PHPAL_I14443P4_SW_PCB_DESELECT          0x00U

/** Bits within PCB which signal a WTX command */
#define PHPAL_I14443P4_SW_PCB_WTX               0x30U

/** WTXM Bits of WTX INF byte */
#define PHPAL_I14443P4_SW_WTXM_BITS             0x3FU

/** FWI default value */
#define PHPAL_I14443P4_SW_FWI_DEFAULT           0x04U

/** FSCI default value */
#define PHPAL_I14443P4_SW_FSCI_DEFAULT          0x02U

/** FTW min in microseconds */
#define PHPAL_I14443P4_SW_FWT_MIN_US            302U

/** FTW max in microseconds */
#define PHPAL_I14443P4_SW_FWT_MAX_US            4949000U

/** FTW max in microseconds in NFC Mode */
#define PHPAL_I14443P4_SW_NFC_FWT_MAX_US        4953000U

/** FTW max in microseconds in EMVCo Mode */
#define PHPAL_I14443P4_SW_EMVCO_FWT_MAX_US      4954000U

/** S(DESELECT) FTW in microseconds */
#define PHPAL_I14443P4_SW_FWT_DESELECT_US       5286U

/** Valid CID bits within CID field */
#define PHPAL_I14443P4_SW_CID_MASK              0x0FU

/** I-Block transmission mode  */
#define PHPAL_I14443P4_SW_STATE_I_BLOCK_TX      0x01U

/** I-Block reception mode  */
#define PHPAL_I14443P4_SW_STATE_I_BLOCK_RX      0x02U

/** Exchange finished mode  */
#define PHPAL_I14443P4_SW_STATE_FINISHED        0x0FU

/** An I-Block mode is retramsmitting */
#define PHPAL_I14443P4_SW_STATE_RETRANSMIT_BIT  0x40U

/** An I-Block mode is chaining */
#define PHPAL_I14443P4_SW_STATE_CHAINING_BIT    0x80U

/** Only 6 bits of the state are pure state codes */
#define PHPAL_I14443P4_SW_STATE_MASK            0x0FU

/** Default Maximum Retry count for ISO/IEC 14443-4:2008(E) Rule 4 and 5 */
#define PHPAL_I14443P4_SW_MAX_RETRIES_DEFAULT   2U

/** Time extension in microseconds for all commands.  */
#define PHPAL_I14443P4_EXT_TIME_US              60U

/** EMVCo: Time extension to support EMVCO Req */
#define PHPAL_I14443P4_EMVCO_FWT_US             30U

/** Power level Indication bit mask for INF of SWTX */
#define PHPAL_I14443P4_SW_S_BLOCK_INF_PLI_MASK 0xC0U

/** FTW min in microseconds */
#define PHPAL_I14443P4_SW_FWT_MIN_US            302U

/** Delta TPCD max in microseconds */
#define PHPAL_I14443P4_SW_DELTA_TPCD_US_MAX     16400U

/** Delta TPCD minimum in microseconds */
#define PHPAL_I14443P4_SW_DELTA_TPCD_US_MIN     0U

/** Delta frame wait time (FWT) in microseconds = 49152/fc */
#if defined(NXPBUILD__PHHAL_HW_RC663)
/** Note: Additional delay is required for RC663 Reader IC to PASS NFC Forum CR11 DP test suite v2.2.1 */
#define PHPAL_I14443P4_SW_DELTA_FWT_US          4100U
#else
#define PHPAL_I14443P4_SW_DELTA_FWT_US          3625U
#endif

/** Delta TPCD used for exchange (in microseconds) */
#define PHPAL_I14443P4_SW_DELTA_TPCD_US         1000U

#ifdef __cplusplus
}
#endif

#endif /* MFRC630_MFRC630_14443P4_H_ */
