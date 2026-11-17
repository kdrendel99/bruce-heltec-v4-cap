# Bruce for Heltec WiFi LoRa 32 V4 + M5Stack Cap CC1101

Full **Bruce** firmware running **natively on the Heltec's onboard 128×64 OLED**,
with Sub-GHz driven by an externally-wired **M5Stack Cap CC1101**. Navigated with
the Heltec's single **PRG** button.

This is a fork of [Yoursel71/BruceButBetter](https://github.com/Yoursel71/BruceButBetter)
with one added board target (`heltec-v4-oled`). The upstream README is preserved as
[`README.BruceButBetter.md`](./README.BruceButBetter.md).

> **Why this exists:** stock Bruce (and the other Heltec fork) only render the full
> UI on color TFTs — on the Heltec's mono OLED they fall back to a limited status
> panel + a web emulator. This build renders the *real* Bruce UI on the OLED itself.

---

## Hardware

| | |
|---|---|
| **Host** | Heltec WiFi LoRa 32 V4 — ESP32-S3**R2** (16 MB flash, **2 MB QSPI** PSRAM), onboard SSD1306 128×64 I²C OLED, single PRG button on GPIO0 |
| **Radio** | M5Stack Cap CC1101 (SKU U219) — TI CC1101 Sub-GHz (315/433/868/915 MHz) + ST25R3916 NFC, 14-pin Cap-Bus, SPI |
| **Power** | USB-C (the Cap is fed from the Heltec's 5V header; this assembly is USB-tethered) |

### Heltec → M5 Cap CC1101 wiring  ← **the important part**

| M5 Cap pad | Heltec pin | Function |
|---|---|---|
| `5V OUT` | 5V (J2-2) | Supply into the Cap's regulator |
| `GND` | GND (J2-1) | Ground |
| `SCK` | **GPIO33** | SPI clock |
| `MOSI` | **GPIO47** | SPI MOSI |
| `MISO` | **GPIO48** | SPI MISO |
| `CC CS` | **GPIO39** | **CC1101 chip-select** (see note) |
| `GDO0` | **GPIO4** | CC1101 GDO0 (RX/TX interrupt) |
| `RF SW0` | **GPIO38** | Antenna-switch control line 0 |

Notes:
- **CC1101 CS is GPIO39.** The Cap's silkscreen swaps the `CC CS` / `NFC CS`
  labels relative to the schematic; GPIO39 is the real CC1101 select (confirmed by
  probing the chip's VERSION register, which reads `0x14`).
- **`RF SW1` is not wired to the Heltec** — it is the CC1101's own `GDO2` pin,
  internal to the Cap, forced high/low in firmware via the `IOCFG2` register.
- **OLED (onboard):** I²C `SDA=GPIO17`, `SCL=GPIO18`, `RST=GPIO21`, panel power via
  **`Vext=GPIO36` (active LOW)** — Vext must be enabled before display init.
- **Deliberate conflicts disabled:** the stock Heltec build routes GPS to 38/39 and
  Grove I²C SCL to 4 — all three collide with the Cap, so GPS/Grove are disabled in
  this build. GPS and the Cap cannot coexist on this wiring.

---

## Software lineage — what we pulled from each project

| Project | What we took |
|---|---|
| [pr3y/Bruce](https://github.com/pr3y/Bruce) (mainline) | The base firmware. v1.17 added *"M5Stack CC1101 + ST25R3916 cap support"* — we ported its **Cap antenna-switch block** (drives `RF_SW0` as a GPIO and forces `RF_SW1`/GDO2 via `IOCFG2`, with the 315/433/868-915 band truth table) into `rf_utils.cpp`. |
| [Unkl3Errl/HeltecFirmware](https://github.com/Unkl3Errl/HeltecFirmware) | A Heltec-V4 Bruce fork. We took the **confirmed Heltec pin constants** (OLED SDA17/SCL18/RST21, Vext36, PRG GPIO0). We did *not* use its architecture — it runs headless and only mirrors the full UI to a web emulator, which is exactly what we wanted to avoid. |
| [Yoursel71/BruceButBetter](https://github.com/Yoursel71/BruceButBetter) | **The base this repo forks.** It already ported Bruce's full UI to a 128×64 mono OLED via **LovyanGFX** (`Panel_SSD1306` + `TFT_MONOCHROME`) plus a **`UI_COMPACT`** core layout (4×2 icon grid, 4-row lists), and already had the `setBeginEndLogic(true)` SPI fix. Our `heltec-v4-oled` board is derived from its `ESP-General` board template. |

---

## What this fork adds

A single new board target, `heltec-v4-oled`:

- `boards/_boards_json/heltec-v4-oled.json` — ESP32-S3, 16 MB flash, `qio_qspi`
  (2 MB QSPI PSRAM). **Must not** define `ESP32S3DEVKITC1` — that key already maps
  to `ESP-General` in the pinouts dispatcher.
- `boards/heltec-v4-oled/pins_arduino.h` — Heltec OLED + Cap CC1101 pins, `DEVICE_NAME`.
- `boards/heltec-v4-oled/heltec-v4-oled.ini` — LovyanGFX mono-OLED config retargeted
  to the Heltec's I²C pins, CC1101 Cap pins + `CAP_CC1101_*` defines, single-button
  flags, `DISABLE_LORA/GPS/ETHERNET/FM`, `lib_ignore = TFT_eSPI FastLED`.
- `boards/heltec-v4-oled/interface.cpp` — Vext power-on, I²C bring-up, CC1101 pin
  seeding + default `rfModule`, and the single-button `InputHandler`.
- `boards/heltec-v4-oled/ok_button.h` — single PRG-button gesture engine
  (**tap = advance, hold ≈0.65 s = select, double-tap = back**).
- `boards/pinouts/pins_arduino.h` — dispatch: `#elif ARDUINO_HELTEC_WIFI_LORA_32_V4`.
- `src/modules/rf/rf_utils.cpp` — the ported Cap antenna-switch block, gated on
  `#ifdef CAP_CC1101_SW0_PIN`.

---

## Gotchas worth remembering

- **CC1101 CS = GPIO39** (Cap silkscreen CS labels are swapped vs schematic).
- **Dedicated SPI bus needs `setBeginEndLogic(true)`.** With a fresh/dedicated bus
  (no other peripheral holding an active transaction), ELECHOUSE must wrap transfers
  in a configured `SPISettings` transaction, or MISO reads `0x00` ("CC1101 not
  found"). BruceButBetter already does this.
- **`lib_ignore = FastLED`** on ESP32-S3 LTO builds — otherwise the link fails with
  `dangerous relocation: windowed longcall crosses 1GB boundary` in FastLED. The
  Heltec has no addressable RGB LED, so FastLED is unused anyway.
- **`RF_SW1` is GDO2**, not a GPIO — set via the CC1101 `IOCFG2` register
  (`0x2F` = forced low, `0x6F` = forced high) per band in `setMHZ()`.
- **`NO_LR_BUTTONS`**: the main icon grid navigates with **UP/DOWN**, not Next/Prev.
  The single-button "advance" therefore emits `DownPress`.
- **Vext (GPIO36, active LOW)** powers the OLED — enable it before display init.
- **R2 silicon → `qio_qspi`.** GPIO33–37 are free general-purpose pins here (unlike
  N16R8/OPI-PSRAM boards where they belong to PSRAM).

---

## Build & flash

```bash
# Build
pio run -e heltec-v4-oled            # or: uvx --from platformio pio run -e heltec-v4-oled

# Flash (board on USB)
pio run -e heltec-v4-oled -t upload
```

Toolchain: PlatformIO with the pinned pioarduino platform (see `platformio.ini`).

**macOS note:** the ESP32-S3's native USB re-enumerates between the app port
(`/dev/cu.usbmodem<serial>`) and the ROM-bootloader port (`/dev/cu.usbmodem1101`)
when it resets for flashing. If a flash/connect fails, retry on whichever
`/dev/cu.usbmodem*` is currently present. `--connect-attempts 15` helps.

---

## Using it

**On the device (single PRG button):**
- **tap** → next item
- **hold (~0.65 s)** → select / enter
- **double-tap** → back

The full Bruce UI renders on the OLED: WiFi, BLE, **RF (Sub-GHz / CC1101)**, IR, etc.

**Over USB serial** (115200 baud, open with DTR asserted; `#` prompt):
```
help                          # list commands
rf rx -frequency 433.92       # -> "cc1101 Connection OK"
rf scan <startHz> <stopHz>    # e.g. rf scan 433000000 434000000
stopscan                      # exit a running RF op
```
Note the Bruce syntax is `rf rx -frequency <MHz>` (flag), not a positional timeout.

**WiFi features caveat:** the ESP32-S3 has one WiFi radio. Launching a WiFi attack
seizes it and tears down any WiFi-served WebUI, so drive WiFi features from the
device or the USB serial CLI, not a browser over the device's own AP.

---

## Recreating this from scratch (high level)

1. Clone `Yoursel71/BruceButBetter`.
2. Add a board `heltec-v4-oled`, copying `boards/ESP-General/` as the template
   (`.json`, `pins_arduino.h`, `.ini`, `interface.cpp`, `ok_button.h`).
3. Retarget the display macros to the Heltec OLED: `Panel_SSD1306`, `TFT_MONOCHROME=1`,
   `UI_COMPACT=1`, `TFT_SDA=17 TFT_SCL=18 TFT_RST=21`, 128×64; power `Vext=36` LOW in
   `_setup_gpio()` before display init.
4. Set the CC1101 pins (`SCK33 MISO48 MOSI47 CS39 GDO0=4`) and
   `CAP_CC1101_SS_PIN=39 / CAP_CC1101_SW0_PIN=38`; port the Cap antenna-switch block
   from `pr3y/Bruce` v1.17's `rf_utils.cpp` (gated on `CAP_CC1101_SW0_PIN`).
5. Implement single-button nav: a tap/hold/double-tap gesture → `DownPress` /
   `SelPress` / `EscPress`.
6. Register the board in `boards/pinouts/pins_arduino.h`
   (`#elif ARDUINO_HELTEC_WIFI_LORA_32_V4`).
7. `lib_ignore = TFT_eSPI` (LovyanGFX replaces it) `FastLED` (longcall link bug).
8. `pio run -e heltec-v4-oled -t upload`.

## Verification status

- ✅ Builds, flashes, boots.
- ✅ CC1101 detected: `cc1101 Connection OK` on 433 / 915 / 315 MHz (antenna switch
  fires per band).
- ✅ Full Bruce UI renders on the 128×64 OLED; single-button nav works.
- ⬜ Live RX decode of a real transmitter / TX / RSSI scan — not yet exercised.
