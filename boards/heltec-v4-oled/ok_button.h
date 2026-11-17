#pragma once
#include <stdint.h>

// Single-button navigation for the Heltec's one PRG button (GPIO0).
//   tap            -> Advance (move to next item)
//   hold >= 650ms  -> Select  (enter / confirm; fires once while held)
//   double-tap     -> Back
// A tap is only emitted after the double-tap window closes, so a double-tap
// never also fires an Advance first.
class OneButtonGesture {
public:
    enum class Action { None, Advance, Select, Back };
    static constexpr uint32_t DebounceMs = 25;
    static constexpr uint32_t HoldMs = 650;
    static constexpr uint32_t DoubleMs = 320; // max gap between taps for a double

    Action update(bool pressed, uint32_t now) {
        // debounce
        if (pressed != rawPressed) {
            rawPressed = pressed;
            rawChangedAt = now;
        }
        if (stablePressed != rawPressed && uint32_t(now - rawChangedAt) >= DebounceMs) {
            stablePressed = rawPressed;
            if (stablePressed) {
                pressedAt = now;
                holdFired = false;
                if (pendingTap && uint32_t(now - lastReleaseAt) <= DoubleMs) {
                    // second press within window -> double-tap == Back
                    pendingTap = false;
                    awaitingSecond = true; // consume the release of this 2nd press
                    return Action::Back;
                }
            } else {
                // released
                if (holdFired) {
                    // hold already emitted Select; swallow the release
                } else if (awaitingSecond) {
                    awaitingSecond = false; // release of the 2nd tap in a double
                } else {
                    // short press released: start double-tap window
                    pendingTap = true;
                    lastReleaseAt = now;
                }
            }
        }
        // hold detection while pressed -> Select
        if (stablePressed && !holdFired && uint32_t(now - pressedAt) >= HoldMs) {
            holdFired = true;
            pendingTap = false;
            return Action::Select;
        }
        // double-tap window expired with a single tap pending -> Advance
        if (pendingTap && !stablePressed && uint32_t(now - lastReleaseAt) > DoubleMs) {
            pendingTap = false;
            return Action::Advance;
        }
        return Action::None;
    }

    bool pressed() const { return stablePressed; }

private:
    bool rawPressed = false;
    bool stablePressed = false;
    bool holdFired = false;
    bool pendingTap = false;
    bool awaitingSecond = false;
    uint32_t rawChangedAt = 0;
    uint32_t pressedAt = 0;
    uint32_t lastReleaseAt = 0;
};
