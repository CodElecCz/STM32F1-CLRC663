---
title: "STM32F103C8Tx + CLRC663: NFC ctecka pres SPI s USB CDC"
description: "Technicky rozbor projektu STM32F1-CLRC663: pinout, hodinova konfigurace, SPI/USB periferie a implementace LPCD, ISO14443A anti-collision a ISO7816-4 APDU."
date: 2026-11-16
lang: cs
tags:
  - stm32
  - stm32f103
  - clrc663
  - mfrc630
  - nfc
  - rfid
  - iso14443
  - iso7816
  - usb-cdc
repo: https://github.com
featured: true
---

# Prehled projektu

Projekt `STM32F1-CLRC663` je firmware pro mikrokontroler **STM32F103C8Tx** s NFC frontendem **NXP CLRC663** (s MFRC630 knihovnou). Aplikace je postavena na STM32 HAL/CubeMX, pouziva SPI pro komunikaci se cteckou a USB CDC (Virtual COM Port) pro diagnosticky vystup.

Architektura je rozdelena do tri vrstev:

1. **Aplikacni vrstva** v `Core/Src/mfrc.c` (`MFRCReaderTask_Init`, `MFRCReaderTask`, `MFRCReader_Dump`)
2. **RFID/NFC protokolova vrstva** v `Drivers/MFRC630/*.c` (ISO14443A, ISO14443-4, APDU)
3. **HW a middleware vrstva** v `Core/Src/main.c` + `USB_DEVICE/App/*`

# Hardware pinout (STM32F103C8Tx -> CLRC663 + USB)

Mapovani podle `STM32F1-CLRC663.ioc` a symbolu v `Core/Inc/main.h`:

| Funkce | Signal na STM32 | Pin STM32 | Poznamka |
|---|---|---:|---|
| SPI1 SCK | `SPI1_SCK` | `PA5` | hodinovy signal SPI |
| SPI1 MISO | `SPI1_MISO` | `PA6` | data CLRC663 -> MCU |
| SPI1 MOSI | `SPI1_MOSI` | `PA7` | data MCU -> CLRC663 |
| CS / NSS (SW) | `SDA` (GPIO output) | `PA2` | softwarove NSS; v `main.c`: `mfrc630_SPI_select/unselect()` ovlada `SDA_Pin` |
| IRQ | `GPIO_EXTI10` (`IRQ`) | `PA10` | preruseni z CLRC663 |
| Reset / PDOWN | `RST` (GPIO output) | `PA0` | reset CLRC663, togglovano pri init |
| USB D- | `USB_DM` | `PA11` | USB FS device |
| USB D+ | `USB_DP` | `PA12` | USB FS device |

Poznamka: hardwarovy SPI NSS pin neni pouzit, `SPI1` bezi s `NSS = SPI_NSS_SOFT`.

# Hodiny a periferie

Konfigurace vychazi z `STM32F1-CLRC663.ioc` a `SystemClock_Config()` v `Core/Src/main.c`:

- Externi oscilator HSE na `PD0/PD1`
- PLL zdroj: HSE, nasobic `x6`
- `SYSCLK = 48 MHz`
- `AHB = 48 MHz`
- `APB1 = 24 MHz` (`HCLK/2`)
- `APB2 = 48 MHz`
- USB clock: `48 MHz` z PLL (`RCC_USBCLKSOURCE_PLL`)

SPI1 je nastaveno jako Master, 2-line, 8bit, Mode 0. V `.ioc` je prescaler `/64` (750 kbit/s), ale uzivatelsky kod v `MX_SPI1_Init()` pouziva prescaler `/8` (komentar "up to 10Mbit/s").

# Firmware moduly a tok protokolu

## 1) LPCD (Low Power Card Detection)

Rizeni LPCD je v `Core/Src/mfrc.c`:

- `MFRCReaderTask_Init(uint8_t lpcd)`
- `MFRCReader_LPCDmeas(uint8_t* i_val, uint8_t* q_val)`
- `MFRCReaderTask(uint8_t lpcd)`

Aplikacni vrstva vola low-level funkce z `Drivers/MFRC630/mfrc630.c`:

- `mfrc630_LPCD_start_measurement()`
- `mfrc630_LPCD_stop_measurement()`
- `mfrc630_LPCD(i_val, q_val)`
- `mfrc630_LPCD_wakup()`

Typicky prubeh: kalibrace I/Q -> nastaveni prahu -> uspani ctecky v LPCD -> probuzeni pri detekci karty.

## 2) ISO14443A (REQA/WUPA + anti-collision + SELECT)

Aplikacni smycka (`MFRCReaderTask` / `MFRCReader_Dump`) pouziva:

- `mfrc630_iso14443a_REQA()` pro detekci karty (ATQA)
- `mfrc630_iso14443a_select(uid, &sak)` pro anti-collision + select
- `mfrc630_iso14443a_WUPA()` pro wake-up po ukonceni vymeny

Samotna anti-collision logika je implementovana v `Drivers/MFRC630/mfrc630.c` (`mfrc630_iso14443a_select`):

- cascade level 1..3
- prace s kolizemi (`MFRC630_REG_RXCOLL`)
- skladani UID a BCC kontrola
- finalni SAK vyhodnoceni

## 3) ISO14443-4 a ISO7816-4 APDU

Pokud `SAK` indikuje podporu ISO14443-4 (bit 5), kod v `Core/Src/mfrc.c` pokracuje:

1. `mfrc630_rats()`
2. `mfrc630_APDU_select_ppse(...)`
3. `mfrc630_APDU_select_app(...)`
4. `mfrc630_14443p4_deselect()`

Implementace:

- `Drivers/MFRC630/mfrc630_14443p4.c`: `mfrc630_14443p4_transfer()`, zpracovani I/R/S bloku, WTX, DESELECT
- `Drivers/MFRC630/mfrc630_apdu.c`: tvorba APDU ramcu `SELECT PPSE` a `SELECT AID`
- `Drivers/MFRC630/mfrc630_apdu.h`: predefinovane AID (`VISA`, `MASTERCARD`) a PPSE

Projekt tak podporuje scenare EMV-like vyberu aplikace i Android HCE testovani.

# USB CDC (Virtual COM Port)

USB stack je ve slozce `USB_DEVICE/`:

- `USB_DEVICE/App/usb_device.c`: `MX_USB_DEVICE_Init()` (USBD init + registrace CDC class)
- `USB_DEVICE/App/usbd_cdc_if.c`: `CDC_Transmit_FS()` a `CDC_Receive_FS()`
- `Core/Src/debug.c`: `_write()` presmerovava `printf` na `CDC_Transmit_FS()` (pokud je aktivni debug konfigurace)

V `Core/Src/main.c` je navic `MX_GPIO_USB_Init()` s kratkym reset pulzem na `PA12` pred startem USB device.

# Build, flash a test

Minimalni postup (STM32CubeIDE):

1. Otevrit projekt `STM32F1-CLRC663`
2. Build (Debug/Release)
3. Flash pres ST-LINK
4. Pripojit USB k desce a otevrit novy COM port
5. Prilozit NFC kartu/telefon a sledovat logy (UID, ATS, APDU odpovedi)

Poznamka: vypisy pres `printf` bezi pres USB CDC pouze pokud je aktivni debug presmerovani v `Core/Src/debug.c` a odpovidajici makra v `Core/Inc/main.h`.

# Kredity a poznamky

- Projekt je zalozen na knihovne iwanders/MFRC630.
- Cilem navrhu je ISO14443A (REQA/WUPA/SELECT s resenim kolizi), ISO14443A-4 wrapper a ISO7816-4 APDU `SELECT PPSE/AID` pro VISA/MASTERCARD a Android HCE testovani.

# Obrazky (placeholder)

![Zapojeni a hardware](./images/hardware-setup.png)

![Vystup terminalu USB CDC](./images/terminal-output.png)