#pragma once
#include "LedDriver.hpp"
#include "hardware.hpp"

struct Context
{
    Relay *relayOpen;
    Relay *relayClose;
    Buzzer *buzzer;
    LedDriver *led;
};
