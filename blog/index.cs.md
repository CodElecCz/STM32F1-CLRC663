---
title: "STM32F103C8Tx + CLRC663: NFC čtečka přes SPI s rozhraním USB CDC"
description: "Technický rozbor projektu STM32F1-CLRC663: zapojení pinů, konfigurace hodinového systému, periferie SPI/USB a implementace LPCD, antikolizního algoritmu ISO14443A a příkazů ISO7816-4 APDU."
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

# Přehled projektu

Projekt `STM32F1-CLRC663` představuje firmware pro mikrokontrolér **STM32F103C8Tx** s multiprotokolovým NFC frontendem **NXP CLRC663** (využívající knihovnu pro MFRC630). Aplikace je postavena na knihovnách STM32 HAL a nástroji CubeMX, ke komunikaci se čtečkou využívá sběrnici SPI a pro diagnostické výpisy rozhraní USB CDC (Virtual COM Port).

Architektura firmwaru je rozdělena do tří vrstev:

1. **Aplikační vrstva** v `Core/Src/mfrc.c` (`MFRCReaderTask_Init`, `MFRCReaderTask`, `MFRCReader_Dump`)
2. **Protokolová vrstva RFID/NFC** v `Drivers/MFRC630/*.c` (ISO14443A, ISO14443-4, APDU)
3. **Hardwarová a middleware vrstva** v `Core/Src/main.c` a `USB_DEVICE/App/*`

# Zapojení hardwaru (STM32F103C8Tx -> CLRC663 + USB)

Mapování pinů vychází ze souboru `STM32F1-CLRC663.ioc` a definic v `Core/Inc/main.h`:

| Funkce | Signál na STM32 | Pin STM32 | Poznámka |
|---|---|---:|---|
| SPI1 SCK | `SPI1_SCK` | `PA5` | hodinový signál sběrnice SPI |
| SPI1 MISO | `SPI1_MISO` | `PA6` | data z CLRC663 do MCU |
| SPI1 MOSI | `SPI1_MOSI` | `PA7` | data z MCU do CLRC663 |
| CS / NSS (SW) | `SDA` (výstup GPIO) | `PA2` | softwarový výběr čipu; funkce `mfrc630_SPI_select/unselect()` v `main.c` ovládají `SDA_Pin` |
| IRQ | `GPIO_EXTI10` (`IRQ`) | `PA10` | externí přerušení z obvodu CLRC663 |
| Reset / PDOWN | `RST` (výstup GPIO) | `PA0` | reset obvodu CLRC663 (přepínáno při inicializaci) |
| USB D- | `USB_DM` | `PA11` | datový vodič USB FS |
| USB D+ | `USB_DP` | `PA12` | datový vodič USB FS |

Poznámka: Hardwarový pin NSS rozhraní SPI není využit, sběrnice `SPI1` pracuje v režimu softwarového řízení (`SPI_NSS_SOFT`).

# Konfigurace hodinového systému a periferií

Nastavení taktování vychází ze souboru `STM32F1-CLRC663.ioc` a funkce `SystemClock_Config()` v `Core/Src/main.c`:

- Externí krystalový oscilátor HSE na pinech `PD0/PD1`
- Zdroj PLL: HSE, násobič `x6`
- `SYSCLK = 48 MHz`
- `AHB = 48 MHz`
- `APB1 = 24 MHz` (`HCLK/2`)
- `APB2 = 48 MHz`
- Hodinový signál pro USB: `48 MHz` z PLL (`RCC_USBCLKSOURCE_PLL`)

Rozhraní SPI1 je nakonfigurováno v režimu Master (dvouvodičově, 8 bitů, SPI Mode 0). V souboru `.ioc` je nastavena předdělička `/64` (750 kbit/s), zatímco uživatelský kód ve funkci `MX_SPI1_Init()` nastavuje předděličku `/8` (s komentářem *„up to 10Mbit/s“*).

# Moduly firmwaru a průběh komunikace

## 1) LPCD (Low Power Card Detection)

Řízení nízkoenergetické detekce karty (LPCD) je implementováno v `Core/Src/mfrc.c`:

- `MFRCReaderTask_Init(uint8_t lpcd)`
- `MFRCReader_LPCDmeas(uint8_t* i_val, uint8_t* q_val)`
- `MFRCReaderTask(uint8_t lpcd)`

Aplikační vrstva volá nízkoúrovňové funkce ze souboru `Drivers/MFRC630/mfrc630.c`:

- `mfrc630_LPCD_start_measurement()`
- `mfrc630_LPCD_stop_measurement()`
- `mfrc630_LPCD(i_val, q_val)`
- `mfrc630_LPCD_wakup()`

Typický průběh: kalibrace složek I/Q -> nastavení detekčních prahů -> uvedení čtečky do úsporného režimu LPCD -> probuzení mikrokontroléru při přiblížení karty.

## 2) ISO14443A (REQA/WUPA, antikolizní algoritmus a SELECT)

Hlavní aplikační smyčka (`MFRCReaderTask` / `MFRCReader_Dump`) využívá následující funkce:

- `mfrc630_iso14443a_REQA()` pro vyhledání karty v poli (ATQA)
- `mfrc630_iso14443a_select(uid, &sak)` pro antikolizní smyčku a výběr konkrétního tagu
- `mfrc630_iso14443a_WUPA()` pro opětovné probuzení karty po ukončení komunikace

Samotná antikolizní logika je implementována v `Drivers/MFRC630/mfrc630.c` (`mfrc630_iso14443a_select`) a zahrnuje:

- průchod kaskádovými úrovněmi 1 až 3 (Cascade Level 1..3),
- vyhodnocení pozice bitové kolize pomocí registru `MFRC630_REG_RXCOLL`,
- sestavení celého UID a kontrolu kontrolního bajtu BCC,
- finální vyhodnocení bajtu SAK.

## 3) ISO14443-4 a příkazy ISO7816-4 APDU

Pokud bit 5 v bajtu `SAK` potvrzuje podporu protokolu ISO14443-4, komunikace v `Core/Src/mfrc.c` pokračuje následující sekvencí:

1. `mfrc630_rats()` (odeslání RATS a příjem ATS)
2. `mfrc630_APDU_select_ppse(...)`
3. `mfrc630_APDU_select_app(...)`
4. `mfrc630_14443p4_deselect()`

Technická implementace je rozdělena do těchto modulů:

- `Drivers/MFRC630/mfrc630_14443p4.c`: funkce `mfrc630_14443p4_transfer()`, obsluha bloků I/R/S, požadavků WTX (Waiting Time Extension) a ukončení relace DESELECT
- `Drivers/MFRC630/mfrc630_apdu.c`: sestavení APDU rámců pro příkazy `SELECT PPSE` a `SELECT AID`
- `Drivers/MFRC630/mfrc630_apdu.h`: definice PPSE a identifikátorů platebních aplikací AID (`VISA`, `MASTERCARD`)

Projekt tak umožňuje testovat výběr aplikací u bezkontaktních platebních karet i emulaci karet v telefonu (Android HCE).

# Rozhraní USB CDC (Virtual COM Port)

USB stack je umístěn ve složce `USB_DEVICE/`:

- `USB_DEVICE/App/usb_device.c`: `MX_USB_DEVICE_Init()` (inicializace USB zařízení a registrace třídy CDC)
- `USB_DEVICE/App/usbd_cdc_if.c`: funkce `CDC_Transmit_FS()` a `CDC_Receive_FS()`
- `Core/Src/debug.c`: funkce `_write()` přesměrovává standardní výstup `printf` do `CDC_Transmit_FS()` (při aktivní ladicí konfiguraci)

V souboru `Core/Src/main.c` je navíc implementována funkce `MX_GPIO_USB_Init()`, která před spuštěním USB periferie vygeneruje krátký resetovací pulz na pinu `PA12` pro spolehlivou re-enumeraci na straně PC.

# Překlad, nahrání a testování

Postup zprovoznění v prostředí STM32CubeIDE:

1. Otevřete projekt `STM32F1-CLRC663`.
2. Spusťte překlad projektu (konfigurace Debug nebo Release).
3. Nahrajte firmware do mikrokontroléru přes programátor ST-LINK.
4. Připojte desku přes USB k počítači a v terminálu otevřete nově vytvořený virtuální COM port.
5. Přiložte ke čtečce NFC kartu nebo telefon a sledujte diagnostické výpisy (UID, ATS a odpovědi APDU).

Poznámka: Výpisy přes funkci `printf` se odesílají přes USB CDC pouze tehdy, je-li povoleno přesměrování ladicích výstupů v `Core/Src/debug.c` a příslušná makra v `Core/Inc/main.h`.

# Zdroje a poděkování

- Firmware vychází z knihovny `iwanders/MFRC630`.
- Cílem návrhu je kompletní podpora ISO14443A (REQA/WUPA/SELECT včetně řešení kolizí), transportní vrstva ISO14443A-4 a výměna APDU příkazů podle ISO7816-4 (`SELECT PPSE/AID`) pro testování karet VISA/MASTERCARD a Android HCE.

# Obrázky

![Zapojení hardwaru](./images/hardware-setup.png)

![Výstup terminálu USB CDC](./images/terminal-output.png)
