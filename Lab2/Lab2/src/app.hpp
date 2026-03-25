#pragma once
#include "stm32f103x6.h"
#include <stdint.h>
#include <string.h>

void utoa10(uint32_t value, char *out);
bool parsePercent(const char *s, uint8_t &out);
uint32_t millis();
void initGpio();
void initTim2FanPwm(uint16_t psc, uint16_t arr);
void initTim3LedPwm(uint16_t psc, uint16_t arr);
void initSysTickTimebase();

class Button
{
public:
    Button(GPIO_TypeDef *port, uint16_t pinMask)
        : port_(port), pinMask_(pinMask) {}
    bool update();

private:
    GPIO_TypeDef *port_;
    uint16_t pinMask_;
    bool lastRaw_ = false;
    bool stable_ = false;
    uint32_t changedAt_ = 0;
};

class PwmChannel
{
public:
    PwmChannel(volatile uint32_t *ccr, uint16_t arr)
        : ccr_(ccr), arr_(arr) {}
    void setPercent(uint8_t percent);

private:
    volatile uint32_t *ccr_;
    uint16_t arr_;
};

class Fan
{
public:
    explicit Fan(PwmChannel &pwm) : pwm_(pwm) {}

    void setSpeed(uint8_t percent);
    uint8_t speed() const;

private:
    PwmChannel &pwm_;
    uint8_t speed_ = 0;
};

class StatusLed
{
public:
    explicit StatusLed(PwmChannel &pwm) : pwm_(pwm) {}

    void setManual();
    void setAuto();
    void setWarning();
    void update();

private:
    PwmChannel &pwm_;
    bool blinking_ = false;
    bool on_ = false;
    uint32_t lastBlinkMs_ = 0;
};

class AdcReader
{
public:
    void init();
    uint16_t getCh1() const;
    uint16_t getCh2() const;
    bool update();
    void onStartConversion();
    void onAdcIrq();

private:
    volatile uint16_t latestCh1Raw_ = 0;
    volatile uint16_t latestCh2Raw_ = 0;

    volatile uint16_t ch1_ = 0;
    volatile uint16_t ch2_ = 0;

    volatile uint8_t seqIndex_ = 0;
    volatile bool conversionBusy_ = false;
    volatile bool dataChanged_ = false;
    volatile bool aggChange_ = false;
};

class Uart1
{
public:
    void init();
    void irqHandler();
    bool hasLine() const;
    bool consumeDroppedLineFlag();
    void readLine(char *out, uint32_t maxLen);
    void sendChar(char c);
    void sendText(const char *s);
    void sendLine(const char *s);
    void sendUInt(uint32_t value);

private:
    volatile char rxBuf_[64] = {};
    volatile uint8_t rxLen_ = 0;
    volatile bool lineReady_ = false;
    volatile bool overflow_ = false;
    volatile bool lineDropped_ = false;
};

enum Mode
{
    MODE_MANUAL = 0,
    MODE_AUTO = 1,
    MODE_WARNING = 2
};

class Controller
{
public:
    Controller(Fan &fan, StatusLed &led, Uart1 &uart)
        : fan_(fan), led_(led), uart_(uart) {}

    void init(uint16_t initialPot, uint16_t initialTemp);
    void onFastTick();
    void onAdcSample(uint16_t potNow, uint16_t tempNow);
    void onPeriodicTick();
    void onModeButton();
    void onAckButton();
    void onUartOverflow();
    void onUartLine();

private:
    static constexpr uint16_t TEMP_LOW_MAX = 1500U;
    static constexpr uint16_t TEMP_MID_MAX = 2800U;
    static constexpr uint16_t TEMP_WARN_ON = 3000U;
    static constexpr uint16_t TEMP_WARN_OFF = 2800U;

    void updateOutputs();
    void sendState();

    Fan &fan_;
    StatusLed &led_;
    Uart1 &uart_;
    Mode mode_ = MODE_MANUAL;
    Mode requestedMode_ = MODE_MANUAL;
    uint16_t potRaw_ = 0;
    uint16_t tempRaw_ = 0;
    bool uartManualOverride_ = false;
    uint8_t uartManualSpeed_ = 0;
};

extern volatile uint32_t g_ms;
extern Uart1 g_uart;
extern AdcReader g_adc;