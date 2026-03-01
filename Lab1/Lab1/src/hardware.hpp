#pragma once
#include "stm32f103x6.h"
#include <functional>

class GpioPin {
public:
    GpioPin(GPIO_TypeDef *port, uint8_t pin);
    void setOutputPushPull();
    void setInputPullUp();
    void setHigh();
    void setLow();
    void toggle();
    bool read();
    uint8_t getPinNumber() const;
    GPIO_TypeDef *getPort() const;

private:
    void enableClock();
    GPIO_TypeDef *port_;
    uint8_t pin_;
};

// Peripheral Classes
class Relay {
public:
    Relay(GpioPin &pin);
    void on();
    void off();
private:
    GpioPin &pin_;
};

class Buzzer {
public:
    Buzzer(GpioPin &pin);
    void on();
    void off();
    void toggle();
private:
    GpioPin &pin_;
};

class RgbLed {
public:
    RgbLed(GpioPin &r, GpioPin &g, GpioPin &b);
    void set(bool r, bool g, bool b);
    void off();
private:
    GpioPin &r_;
    GpioPin &g_;
    GpioPin &b_;
};

// Interrupt Driven Button
using ButtonCallback = std::function<void()>;

class Button {
public:
    Button(GpioPin &pin);
    void setCallback(ButtonCallback cb);
    void handleInterrupt();

private:
    void setupExti();
    void enableIRQ(uint8_t pin);
    GpioPin &pin_;
    ButtonCallback callback_;
    uint32_t lastPressTime_;
};

// System Functions
void TIMER_Init_1ms(void);
extern volatile uint32_t msTicks;

// Global Hardware Instances
extern Button btnOpen;
extern Button btnClose;
extern Button btnService;
extern Button moveDetector;
extern Relay relayOpen;
extern Relay relayClose;
extern Buzzer buzzer;
extern RgbLed led;