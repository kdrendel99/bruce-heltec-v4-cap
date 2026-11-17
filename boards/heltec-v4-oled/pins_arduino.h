#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include "soc/soc_caps.h"
#include <stdint.h>

// ── Heltec WiFi LoRa 32 V4 + M5Stack Cap CC1101 ──────────────────────────────
// Native full-Bruce UI on the onboard 128x64 SSD1306 OLED (via LovyanGFX mono),
// Sub-GHz on the externally-wired M5 Cap CC1101. Single PRG button (GPIO0).

#define USB_VID 0x303A
#define USB_PID 0x1001
#define USB_as_HID 1

#define DEVICE_NAME "\"Heltec-V4-Cap\""
#define BTN_ALIAS "\"PRG\""

static const uint8_t TX = 43;
static const uint8_t RX = 44;
#define SERIAL_TX 43
#define SERIAL_RX 44

// ── OLED I2C (onboard SSD1306): SDA=17, SCL=18, RST=21, powered via Vext=36 ───
#define GROVE_SDA 17
#define GROVE_SCL 18
static const uint8_t SDA = GROVE_SDA;
static const uint8_t SCL = GROVE_SCL;

// ── CC1101 bus (M5 Cap, external wiring, proven) ─────────────────────────────
// SCK=33 MISO=48 MOSI=47 CS=39 GDO0=4 ; RF_SW0=38 (Cap antenna switch)
#define SPI_MOSI_PIN 47
#define SPI_SCK_PIN  33
#define SPI_MISO_PIN 48
#define SPI_SS_PIN   39
static const uint8_t MOSI = SPI_MOSI_PIN;
static const uint8_t MISO = SPI_MISO_PIN;
static const uint8_t SCK  = SPI_SCK_PIN;
static const uint8_t SS   = SPI_SS_PIN;

#ifndef USE_CC1101_VIA_SPI
#define USE_CC1101_VIA_SPI
#endif
#define CC1101_GDO0_PIN  4
#define CC1101_SS_PIN    39
#ifndef CC1101_MOSI_PIN
#define CC1101_MOSI_PIN  SPI_MOSI_PIN
#endif
#ifndef CC1101_SCK_PIN
#define CC1101_SCK_PIN   SPI_SCK_PIN
#endif
#ifndef CC1101_MISO_PIN
#define CC1101_MISO_PIN  SPI_MISO_PIN
#endif

#endif // Pins_Arduino_h
