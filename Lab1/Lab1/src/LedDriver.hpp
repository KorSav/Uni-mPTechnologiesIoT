#pragma once
#include "hardware.hpp"

class LedDriver
{
public:
    LedDriver(RgbLed &led) : led(led) {}
    void enable(bool en)
    {
        enabled_ = en;
        if (!enabled_)
            led.off();
    }
    void set(bool r, bool g, bool b)
    {
        if (enabled_)
            led.set(r, g, b);
    }
    void off()
    {
        led.off();
    }

private:
    bool enabled_ = true;
    RgbLed &led;
};