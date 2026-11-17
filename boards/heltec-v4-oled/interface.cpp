#include "core/display.h"
#include "core/powerSave.h"
#include "core/utils.h"
#include "ok_button.h"
#include <Wire.h>
#include <driver/rtc_io.h>
#include <interface.h>

#ifndef VEXT_PIN
#define VEXT_PIN 36
#endif

/***************************************************************************************
** _setup_gpio() - called before tft.init()
***************************************************************************************/
void _setup_gpio() {
    // Vext powers the onboard OLED on the Heltec V4 and is active-LOW. It must be
    // enabled before the I2C bus / display are brought up or the panel stays dark.
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, LOW);
    delay(60);

    // OLED I2C bus (SSD1306 @ 0x3C). LovyanGFX drives the panel; Wire is also
    // started so i2c_finder and any future I2C peripherals work.
    Wire.begin(GROVE_SDA, GROVE_SCL);
    Wire.setClock(400000);

    // Hold the CC1101 chip-select high to avoid bus contention before first use.
    pinMode(CC1101_SS_PIN, OUTPUT);
    digitalWrite(CC1101_SS_PIN, HIGH);

    // Single PRG button, active-LOW with internal pull-up.
    pinMode(SEL_BTN, INPUT_PULLUP);

    // Default RF module is the CC1101 on the M5 Cap.
    bruceConfigPins.rfModule = CC1101_SPI_MODULE;

    // Seed the CC1101 SPI pins (proven M5 Cap wiring) so Sub-GHz works out of the
    // box. CS must equal CAP_CC1101_SS_PIN (39) for the antenna-switch block.
    bruceConfigPins.CC1101_bus.sck = (gpio_num_t)CC1101_SCK_PIN;
    bruceConfigPins.CC1101_bus.miso = (gpio_num_t)CC1101_MISO_PIN;
    bruceConfigPins.CC1101_bus.mosi = (gpio_num_t)CC1101_MOSI_PIN;
    bruceConfigPins.CC1101_bus.cs = (gpio_num_t)CC1101_SS_PIN;
    bruceConfigPins.CC1101_bus.io0 = (gpio_num_t)CC1101_GDO0_PIN;
}

/***************************************************************************************
** Battery / brightness - Heltec V4 runs from USB here
***************************************************************************************/
int getBattery() { return 0; }
bool isCharging() { return false; }
void _setBrightness(uint8_t brightval) { (void)brightval; }

/***************************************************************************************
** InputHandler() - single PRG button
**   tap=Next (advance)  double-tap=Select  hold=Back
***************************************************************************************/
void InputHandler(void) {
    static OneButtonGesture btn;
    static bool wasDown = false;
    static bool suppress = false;
    const uint32_t now = millis();

    const auto action = btn.update(digitalRead(SEL_BTN) == BTN_ACT, now);

    // First touch after the screen dimmed just wakes it; don't also navigate.
    if (btn.pressed() && !wasDown) suppress = wakeUpScreen();
    wasDown = btn.pressed();
    if (!btn.pressed()) { if (action == OneButtonGesture::Action::None) suppress = false; }

    if (action == OneButtonGesture::Action::None) return;
    if (suppress || wakeUpScreen()) return;

    AnyKeyPress = true;
    switch (action) {
        // Grid + lists advance on DownPress (NO_LR_BUTTONS maps the grid to UP/DOWN,
        // and feature screens remap their LEFT/RIGHT onto UP/DOWN too).
        case OneButtonGesture::Action::Advance: DownPress = true; break;
        case OneButtonGesture::Action::Select: SelPress = true; break;
        case OneButtonGesture::Action::Back: EscPress = true; break;
        default: break;
    }
}

/***************************************************************************************
** powerOff() - deep sleep, wake on PRG (GPIO0 is RTC-capable on the S3)
***************************************************************************************/
void powerOff() {
    const gpio_num_t wakePin = static_cast<gpio_num_t>(SEL_BTN);
    tft.fillScreen(TFT_BLACK);
    while (digitalRead(SEL_BTN) == BTN_ACT) delay(10);
    if (esp_sleep_enable_ext0_wakeup(wakePin, 0) != ESP_OK) return;
    rtc_gpio_pullup_en(wakePin);
    rtc_gpio_pulldown_dis(wakePin);
    esp_deep_sleep_start();
}

/***************************************************************************************
** checkReboot() - no dedicated button combo on a single-button device.
***************************************************************************************/
void checkReboot() {}
