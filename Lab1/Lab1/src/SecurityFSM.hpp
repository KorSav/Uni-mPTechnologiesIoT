#pragma once
#include "Context.hpp"

class SecurityFSM
{
public:
    enum class State
    {
        Idle,
        Warning,
        Alarm
    };
    SecurityFSM(Context *ctx) : ctx_(ctx) {}

    void reset()
    {
        state_ = State::Idle;
        ctx_->led->off();
        ctx_->buzzer->off();
        blinkAt_ = 0;
        buzToggle_ = 0;
        warningEnd_ = 0;
    }

    void motion(uint32_t now)
    {
        if (state_ == State::Idle)
        {
            state_ = State::Warning;
            warningEnd_ = now + 6000;
            blinkAt_ = now;
        }
        else if (state_ == State::Warning)
        {
            state_ = State::Alarm;
            blinkAt_ = buzToggle_ = now;
        }
    }

    void update(uint32_t now)
    {
        switch (state_)
        {
        case State::Idle:
            break;
        case State::Warning:
            if (now >= warningEnd_)
            {
                reset();
                break;
            }
            if (now >= blinkAt_)
            {
                blink_ = !blink_;
                blinkAt_ = now + 500;
                if (blink_)
                    ctx_->led->set(1, 1, 0);
                else
                    ctx_->led->off();
            }
            break;
        case State::Alarm:
            if (now >= blinkAt_)
            {
                blink_ = !blink_;
                blinkAt_ = now + 200;
                if (blink_)
                    ctx_->led->set(1, 0, 0);
                else
                    ctx_->led->off();
            }
            if (buzToggle_ && now >= buzToggle_)
            {
                ctx_->buzzer->toggle();
                buzToggle_ = now + 150;
            }
            break;
        }
    }

    State state() const { return state_; }

private:
    Context *ctx_;
    State state_ = State::Idle;
    uint32_t warningEnd_ = 0;
    uint32_t blinkAt_ = 0;
    uint32_t buzToggle_ = 0;
    bool blink_ = false;
};