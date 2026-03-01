#pragma once
#include "Context.hpp"

class ServiceMode
{
public:
    ServiceMode(Context *ctx) : ctx_(ctx) {}

    void press(uint32_t now)
    {
        presses_[count_ % 3] = now;
        count_++;
        if (count_ >= 3)
        {
            uint32_t first = presses_[(count_ - 3) % 3];
            if (now - first <= 900)
            {
                active_ = true;
                end_ = now + 10000;
                ctx_->led->enable(false);
                ctx_->buzzer->on();
                buzOff_ = now + 500;
            }
        }
    }

    void update(uint32_t now)
    {
        if (active_ && now >= end_)
        {
            active_ = false;
            ctx_->led->enable(true);
        }
        if (buzOff_ && now >= buzOff_)
        {
            ctx_->buzzer->off();
            buzOff_ = 0;
        }
    }

private:
    Context *ctx_;
    bool active_ = false;
    uint32_t end_ = 0;
    uint32_t buzOff_ = 0;
    uint32_t presses_[3];
    uint8_t count_ = 0;
};