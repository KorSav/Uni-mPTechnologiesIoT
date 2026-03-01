#pragma once
#include "hardware.hpp"
#include "LedDriver.hpp"
#include "ServiceMode.hpp"
#include "SecurityFSM.hpp"
#include <stdint.h>

class IState
{
public:
    virtual void enter(uint32_t now) = 0;
    virtual void exit(uint32_t now) = 0;
    virtual void update(uint32_t now) = 0;
    virtual ~IState() = default;
};

class OpenState : public IState
{
public:
    OpenState(Context *ctx) : ctx_(ctx) {}
    void enter(uint32_t now) override
    {
        ctx_->relayOpen->on();
        relayOff_ = now + 250;
        ctx_->buzzer->on();
        buzOff_ = now + 150;
    }
    void update(uint32_t now) override
    {
        ctx_->led->set(0, 1, 0);
        if (relayOff_ && now >= relayOff_)
        {
            ctx_->relayOpen->off();
            relayOff_ = 0;
        }
        if (buzOff_ && now >= buzOff_)
        {
            ctx_->buzzer->off();
            buzOff_ = 0;
        }
    }
    void exit(uint32_t) override
    {
        ctx_->relayOpen->off();
        ctx_->relayClose->off();
        ctx_->buzzer->off();
        ctx_->led->off();
    }

private:
    Context *ctx_;
    uint32_t relayOff_ = 0, buzOff_ = 0;
};

class ClosedState : public IState
{
public:
    ClosedState(Context *ctx) : ctx_(ctx), security_(ctx) {}

    void enter(uint32_t now) override
    {
        security_.reset();
        ctx_->relayClose->on();
        relayOff_ = now + 250;
        ctx_->buzzer->on();
        buzOff_ = now + 400;
        flashAt_ = now;
    }

    void update(uint32_t now) override
    {
        if (relayOff_ && now >= relayOff_)
        {
            ctx_->relayClose->off();
            relayOff_ = 0;
        }
        if (buzOff_ && now >= buzOff_)
        {
            ctx_->buzzer->off();
            buzOff_ = 0;
        }
        security_.update(now);

        if (securityIdle() && now >= flashAt_)
        {
            ctx_->led->set(1, 0, 0);
            redOff_ = now + 200;
            flashAt_ = now + 3000;
        }
        if (redOff_ && now >= redOff_)
        {
            ctx_->led->off();
            redOff_ = 0;
        }
    }

    void exit(uint32_t) override
    {
        security_.reset();
        ctx_->relayClose->off();
        ctx_->buzzer->off();
        ctx_->led->off();
    }

    void motion(uint32_t now) { security_.motion(now); }

private:
    Context *ctx_;
    SecurityFSM security_;
    uint32_t relayOff_ = 0, buzOff_ = 0, flashAt_ = 0, redOff_ = 0;
    bool securityIdle() const { return security_.state() == SecurityFSM::State::Idle; }
};

class System
{
public:
    Context ctx;
    OpenState open;
    ClosedState closed;
    ServiceMode service;
    IState *current;

    System(Relay &openR, Relay &closeR, Buzzer &buz, LedDriver &led)
        : ctx{&openR, &closeR, &buz, &led}, open(&ctx), closed(&ctx), service(&ctx)
    {
        current = &closed;
    }

    void init(uint32_t now) { current->enter(now); }
    void update(uint32_t now)
    {
        service.update(now);
        current->update(now);
    }
    void onOpen() { transition(&open, msTicks); }
    void onClose() { transition(&closed, msTicks); }
    void onMotion() { closed.motion(msTicks); }
    void onService() { service.press(msTicks); }

private:
    void transition(IState *next, uint32_t now)
    {
        current->exit(now);
        current = next;
        current->enter(now);
    }
};