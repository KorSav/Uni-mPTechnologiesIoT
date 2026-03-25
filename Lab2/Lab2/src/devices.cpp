#include <app.hpp>

bool Button::update()
{
    bool rawPressed = ((port_->IDR & pinMask_) == 0U);

    if (rawPressed != lastRaw_)
    {
        lastRaw_ = rawPressed;
        changedAt_ = millis();
    }

    if ((millis() - changedAt_) >= 30U)
    {
        if (stable_ != rawPressed)
        {
            stable_ = rawPressed;
            if (stable_)
                return true;
        }
    }

    return false;
}

void PwmChannel::setPercent(uint8_t percent)
{
    if (percent > 100U)
        percent = 100U;

    uint32_t value = ((uint32_t)(arr_ + 1U) * percent) / 100U;
    if (value > arr_)
        value = arr_;
    *ccr_ = value;
}

void Fan::setSpeed(uint8_t percent)
{
    speed_ = percent;
    pwm_.setPercent(percent);
}

uint8_t Fan::speed() const
{
    return speed_;
}

void StatusLed::setManual()
{
    blinking_ = false;
    on_ = true;
    pwm_.setPercent(50U);
}

void StatusLed::setAuto()
{
    blinking_ = false;
    on_ = true;
    pwm_.setPercent(80U);
}

void StatusLed::setWarning()
{
    blinking_ = true;
    on_ = true;
    pwm_.setPercent(100U);
    lastBlinkMs_ = millis();
}

void StatusLed::update()
{
    if (!blinking_)
        return;

    if ((millis() - lastBlinkMs_) >= 250U)
    {
        lastBlinkMs_ = millis();
        on_ = !on_;
        pwm_.setPercent(on_ ? 100U : 0U);
    }
}