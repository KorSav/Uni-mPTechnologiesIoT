#include "hardware.hpp"

Button *extiDispatch[16] = {nullptr};

// GpioPin Implementation
GpioPin::GpioPin(GPIO_TypeDef *port, uint8_t pin) : port_(port), pin_(pin) {
    enableClock();
}

void GpioPin::setOutputPushPull() {
    volatile uint32_t *configReg = (pin_ < 8) ? &port_->CRL : &port_->CRH;
    uint8_t shift = (pin_ % 8) * 4;
    *configReg &= ~(0xF << shift);
    *configReg |= (0x1 << shift);       // MODE = 01 (10 MHz output)
    *configReg |= (0x0 << (shift + 2)); // CNF = 00 (push-pull)
}

void GpioPin::setInputPullUp() {
    volatile uint32_t *configReg = (pin_ < 8) ? &port_->CRL : &port_->CRH;
    uint8_t shift = (pin_ % 8) * 4;
    *configReg &= ~(0xF << shift);
    *configReg |= (0x8 << shift); // MODE=00, CNF=10 (input PU/PD)
    setHigh();
}

void GpioPin::setHigh() { port_->BSRR = (1 << pin_); }
void GpioPin::setLow() { port_->BRR = (1 << pin_); }
void GpioPin::toggle() { port_->ODR ^= (1 << pin_); }
bool GpioPin::read() { return (port_->IDR & (1 << pin_)); }
uint8_t GpioPin::getPinNumber() const { return pin_; }
GPIO_TypeDef *GpioPin::getPort() const { return port_; }

void GpioPin::enableClock() {
    if (port_ == GPIOA) RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    if (port_ == GPIOB) RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    if (port_ == GPIOC) RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
}

// Relay/Buzzer/LED Implementation
Relay::Relay(GpioPin &pin) : pin_(pin) { pin_.setOutputPushPull(); }
void Relay::on() { pin_.setHigh(); }
void Relay::off() { pin_.setLow(); }

Buzzer::Buzzer(GpioPin &pin) : pin_(pin) { pin_.setOutputPushPull(); }
void Buzzer::on() { pin_.setHigh(); }
void Buzzer::off() { pin_.setLow(); }
void Buzzer::toggle() { pin_.toggle(); }

RgbLed::RgbLed(GpioPin &r, GpioPin &g, GpioPin &b) : r_(r), g_(g), b_(b) {
    r_.setOutputPushPull();
    g_.setOutputPushPull();
    b_.setOutputPushPull();
}
void RgbLed::set(bool r, bool g, bool b) {
    r ? r_.setHigh() : r_.setLow();
    g ? g_.setHigh() : g_.setLow();
    b ? b_.setHigh() : b_.setLow();
}
void RgbLed::off() { set(false, false, false); }

// Button Implementation
Button::Button(GpioPin &pin) : pin_(pin), callback_(nullptr) {
    pin_.setInputPullUp();
    setupExti();
    extiDispatch[pin_.getPinNumber()] = this;
}

void Button::setCallback(ButtonCallback cb) { callback_ = cb; }

void Button::handleInterrupt() {
    uint32_t now = msTicks;
    if (now - lastPressTime_ <= 30) return;
    lastPressTime_ = now;
    if (callback_) callback_();
}

void Button::setupExti() {
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    uint8_t portSource = (pin_.getPort() == GPIOA) ? 0 : 
                         (pin_.getPort() == GPIOB) ? 1 : 
                         (pin_.getPort() == GPIOC) ? 2 : 0;
    uint8_t pin = pin_.getPinNumber();
    AFIO->EXTICR[pin / 4] &= ~(0xF << ((pin % 4) * 4));
    AFIO->EXTICR[pin / 4] |= (portSource << ((pin % 4) * 4));
    EXTI->IMR |= (1 << pin);
    EXTI->FTSR |= (1 << pin); // falling edge
    enableIRQ(pin);
}

void Button::enableIRQ(uint8_t pin) {
    if (pin <= 4) NVIC_EnableIRQ(static_cast<IRQn_Type>(EXTI0_IRQn + pin));
    else if (pin <= 9) NVIC_EnableIRQ(EXTI9_5_IRQn);
    else NVIC_EnableIRQ(EXTI15_10_IRQn);
}

// Interrupt Handlers
void handleLine(uint8_t line) {
    EXTI->PR |= (1 << line);
    if (extiDispatch[line]) extiDispatch[line]->handleInterrupt();
}

extern "C" {
    void EXTI0_IRQHandler() { handleLine(0); }
    void EXTI1_IRQHandler() { handleLine(1); }
    void EXTI2_IRQHandler() { handleLine(2); }
    void EXTI9_5_IRQHandler() {
        for (int line = 5; line <= 9; ++line) {
            if (EXTI->PR & (1 << line)) handleLine(line);
        }
    }
    void SysTick_Handler(void) { msTicks++; }
}

void TIMER_Init_1ms(void) {
    SysTick->LOAD = 8000 - 1; 
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

volatile uint32_t msTicks = 0;

// Global Instance Definitions
GpioPin _btn1pin(GPIOA, 0);
Button btnOpen(_btn1pin);

GpioPin _btn2pin(GPIOA, 1);
Button btnClose(_btn2pin);

GpioPin _btn3pin(GPIOA, 2);
Button btnService(_btn3pin);

GpioPin _mvDtorPin(GPIOA, 7);
Button moveDetector(_mvDtorPin);

GpioPin _relay1pin(GPIOB, 0);
Relay relayOpen(_relay1pin);

GpioPin _relay2pin(GPIOB, 1);
Relay relayClose(_relay2pin);

GpioPin _buzzerPin(GPIOB, 4);
Buzzer buzzer(_buzzerPin);

GpioPin _redPin(GPIOB, 5);
GpioPin _greenPin(GPIOB, 6);
GpioPin _bluePin(GPIOB, 7);
RgbLed led(_redPin, _greenPin, _bluePin);