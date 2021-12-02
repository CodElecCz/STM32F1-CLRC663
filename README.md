# STM32F1-HAL
Implementation STM32F1 HAL with CubeMX and CubeIDE

## STM32F103C8TX
    64kB of Flash memory
    20 Kbytes of SRAM including
    
## Peripheries
    CLRC663 - High performance multi-protocol NFC frontend       

### CubeMx
    HAL    
    SYSCLK (48Mhz)
    GPIO    
    USB_OTG_FS	
	
#### Middleware    
    USB_DEVICE (Virtual Port Com)
    
## Design
Based on iwanders/MFRC630 library
* ISO14443A: Provides the necessities from ISO14443a to interact with RFID tags: the REQA, WUPA and SELECT procedure (with collision handling) to determine the UID(s).
* ISO14443A-4: wrapper for APDU commands
* ISO7816-4 APDU: commands for SELECT PPSE/AID - working with VISA/MASTERCARD, support for HCE on Andriod (HCE Test Tool)