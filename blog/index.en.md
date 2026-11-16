---
title: "STM32F103C8Tx + CLRC663: SPI NFC Reader with USB CDC"
description: "Technical deep-dive into STM32F1-CLRC663: exact pinout, clock setup, SPI/USB peripherals, and LPCD + ISO14443A anti-collision + ISO7816-4 APDU flow."
date: 2026-11-16
lang: en
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

# Project overview

`STM32F1-CLRC663` is an NFC/RFID firmware project for **STM32F103C8Tx** using an **NXP CLRC663** frontend (through MFRC630-compatible driver API). The firmware is generated from CubeMX/HAL, uses SPI for reader communication, and exposes debug output over USB CDC (Virtual COM Port).

The software architecture is split into three layers:

1. **Application layer** in `Core/Src/mfrc.c` (`MFRCReaderTask_Init`, `MFRCReaderTask`, `MFRCReader_Dump`)
2. **Protocol/reader layer** in `Drivers/MFRC630/*.c` (ISO14443A, ISO14443-4, APDU)
3. **Hardware + middleware layer** in `Core/Src/main.c` and `USB_DEVICE/App/*`

# Hardware pinout (STM32F103C8Tx -> CLRC663 + USB)

Pin mapping extracted from `STM32F1-CLRC663.ioc` and `Core/Inc/main.h`:

| Function | STM32 Signal | STM32 Pin | Note |
|---|---|---:|---|
| SPI1 SCK | `SPI1_SCK` | `PA5` | SPI clock |
| SPI1 MISO | `SPI1_MISO` | `PA6` | CLRC663 -> MCU data |
| SPI1 MOSI | `SPI1_MOSI` | `PA7` | MCU -> CLRC663 data |
| CS / NSS (software) | `SDA` (GPIO output) | `PA2` | software chip-select; controlled by `mfrc630_SPI_select/unselect()` in `main.c` |
| IRQ | `GPIO_EXTI10` (`IRQ`) | `PA10` | external interrupt from CLRC663 |
| Reset / PDOWN | `RST` (GPIO output) | `PA0` | CLRC663 reset line |
| USB D- | `USB_DM` | `PA11` | USB FS device |
| USB D+ | `USB_DP` | `PA12` | USB FS device |

Note: hardware SPI NSS is not used (`SPI_NSS_SOFT`).

# Clock and peripheral configuration

From `STM32F1-CLRC663.ioc` and `SystemClock_Config()` in `Core/Src/main.c`:

- External HSE oscillator on `PD0/PD1`
- PLL source: HSE, multiplier `x6`
- `SYSCLK = 48 MHz`
- `AHB = 48 MHz`
- `APB1 = 24 MHz` (`HCLK/2`)
- `APB2 = 48 MHz`
- USB clock: `48 MHz` from PLL (`RCC_USBCLKSOURCE_PLL`)

SPI1 is configured as Master, 2-line, 8-bit, mode 0. The `.ioc` metadata shows prescaler `/64` (750 kbit/s), while user code in `MX_SPI1_Init()` uses prescaler `/8` (commented as "up to 10Mbit/s").

# Firmware modules and protocol flow

## 1) LPCD wake-up flow

LPCD orchestration is in `Core/Src/mfrc.c`:

- `MFRCReaderTask_Init(uint8_t lpcd)`
- `MFRCReader_LPCDmeas(uint8_t* i_val, uint8_t* q_val)`
- `MFRCReaderTask(uint8_t lpcd)`

These call low-level LPCD functions in `Drivers/MFRC630/mfrc630.c`:

- `mfrc630_LPCD_start_measurement()`
- `mfrc630_LPCD_stop_measurement()`
- `mfrc630_LPCD(i_val, q_val)`
- `mfrc630_LPCD_wakup()`

Typical sequence: measure baseline I/Q -> configure thresholds -> enter LPCD low-power mode -> wake when a card is detected.

## 2) ISO14443A polling and anti-collision

In `MFRCReaderTask`/`MFRCReader_Dump` (`Core/Src/mfrc.c`), the polling flow is:

- `mfrc630_iso14443a_REQA()`
- `mfrc630_iso14443a_select(uid, &sak)`
- `mfrc630_iso14443a_WUPA()`

The anti-collision/select algorithm itself is implemented in `Drivers/MFRC630/mfrc630.c` (`mfrc630_iso14443a_select`), including:

- cascade levels 1..3
- collision position handling via `MFRC630_REG_RXCOLL`
- UID assembly + BCC check
- final SAK evaluation

## 3) ISO14443-4 + ISO7816-4 APDU exchange

When `SAK` bit 5 indicates ISO14443-4 support, `Core/Src/mfrc.c` executes:

1. `mfrc630_rats()`
2. `mfrc630_APDU_select_ppse(...)`
3. `mfrc630_APDU_select_app(...)`
4. `mfrc630_14443p4_deselect()`

Implementation details:

- `Drivers/MFRC630/mfrc630_14443p4.c`: `mfrc630_14443p4_transfer()` with I/R/S block handling, WTX, DESELECT
- `Drivers/MFRC630/mfrc630_apdu.c`: APDU frame building for `SELECT PPSE` and `SELECT AID`
- `Drivers/MFRC630/mfrc630_apdu.h`: constants for PPSE and payment AIDs (`VISA`, `MASTERCARD`)

This makes the project suitable for payment card experiments and Android HCE interoperability tests.

# USB CDC debug interface

USB device middleware is located in `USB_DEVICE/`:

- `USB_DEVICE/App/usb_device.c`: `MX_USB_DEVICE_Init()` (USBD init, CDC class registration)
- `USB_DEVICE/App/usbd_cdc_if.c`: `CDC_Transmit_FS()` and `CDC_Receive_FS()`
- `Core/Src/debug.c`: `_write()` routes `printf` to `CDC_Transmit_FS()` when debug macros are enabled

`Core/Src/main.c` also includes `MX_GPIO_USB_Init()` that pulses `PA12` before starting USB device initialization.

# Build, flash, and test

Minimal practical workflow in STM32CubeIDE:

1. Open the `STM32F1-CLRC663` project
2. Build (Debug/Release)
3. Flash via ST-LINK
4. Connect USB and open the enumerated Virtual COM port
5. Present a card or Android HCE device and observe UID/ATS/APDU logs

Note: `printf` over USB CDC depends on debug redirection in `Core/Src/debug.c` and compile-time macros in `Core/Inc/main.h`.

# Credits and notes

- Based on iwanders/MFRC630 library.
- Design goal includes ISO14443A (REQA/WUPA/SELECT with collision handling), ISO14443A-4 wrapper behavior, and ISO7816-4 APDU `SELECT PPSE/AID` flows for VISA/MASTERCARD and Android HCE testing.

# Image placeholders

![Hardware setup](./images/hardware-setup.png)

![USB CDC terminal output](./images/terminal-output.png)
