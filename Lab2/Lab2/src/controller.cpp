#include <app.hpp>

void Controller::init(uint16_t initialPot, uint16_t initialTemp)
{
    potRaw_ = initialPot;
    tempRaw_ = initialTemp;
    mode_ = MODE_MANUAL;
    requestedMode_ = MODE_MANUAL;
    updateOutputs();
    uart_.sendLine("=== SYSTEM STARTED ===");
    uart_.sendLine("Mode: MANUAL (default)");
    uart_.sendLine("Buttons: PB0=MODE, PB1=ACK");
    uart_.sendLine("Commands: mode manual | mode auto | set N | pot | status | ack");
    sendState();
}

void Controller::onFastTick()
{
    led_.update();
}

void Controller::onAdcSample(uint16_t potNow, uint16_t tempNow)
{
    potRaw_ = potNow;
    tempRaw_ = tempNow;

    if (mode_ != MODE_WARNING && tempRaw_ >= TEMP_WARN_ON)
    {
        mode_ = MODE_WARNING;
        updateOutputs();
        uart_.sendLine("WARN:TEMP_HIGH");
        sendState();
        return;
    }

    if (mode_ != MODE_WARNING)
    {
        mode_ = requestedMode_;
        updateOutputs();
    }
}

void Controller::onPeriodicTick()
{
    if (mode_ == MODE_WARNING)
        uart_.sendLine("WARN:TEMP_HIGH");
}

void Controller::onModeButton()
{
    if (mode_ == MODE_WARNING)
        return;
    if (mode_ == MODE_MANUAL)
        requestedMode_ = MODE_AUTO;
    else
        requestedMode_ = MODE_MANUAL;
    mode_ = requestedMode_;
    sendState();
    updateOutputs();
}

void Controller::onAckButton()
{
    if (mode_ == MODE_WARNING && tempRaw_ < TEMP_WARN_OFF)
    {
        mode_ = requestedMode_;
        updateOutputs();
        uart_.sendLine("OK:WARNING_CLEARED");
        sendState();
    }
}

void Controller::onUartOverflow()
{
    uart_.sendLine("err:line_too_long");
}

void Controller::onUartLine()
{
    char cmd[64];
    uart_.readLine(cmd, sizeof(cmd));

    uart_.sendText("cmd:");
    uart_.sendLine(cmd);

    if (strcmp(cmd, "mode manual") == 0)
    {
        requestedMode_ = MODE_MANUAL;
        if (mode_ != MODE_WARNING)
        {
            mode_ = MODE_MANUAL;
            updateOutputs();
        }
        uart_.sendLine("ok");
        sendState();
        return;
    }

    if (strcmp(cmd, "mode auto") == 0)
    {
        requestedMode_ = MODE_AUTO;
        if (mode_ != MODE_WARNING)
        {
            mode_ = MODE_AUTO;
            updateOutputs();
        }
        uart_.sendLine("ok");
        sendState();
        return;
    }

    if (strncmp(cmd, "set ", 4) == 0)
    {
        uint8_t value = 0;
        if (!parsePercent(cmd + 4, value))
        {
            uart_.sendLine("err");
            return;
        }

        uartManualOverride_ = true;
        uartManualSpeed_ = value;
        if (mode_ == MODE_MANUAL)
            updateOutputs();

        uart_.sendLine("ok");
        sendState();
        return;
    }

    if (strcmp(cmd, "pot") == 0)
    {
        uartManualOverride_ = false;
        if (mode_ == MODE_MANUAL)
            updateOutputs();

        uart_.sendLine("ok");
        sendState();
        return;
    }

    if (strcmp(cmd, "status") == 0)
    {
        sendState();
        return;
    }

    if (strcmp(cmd, "ack") == 0)
    {
        onAckButton();
        return;
    }

    uart_.sendLine("err");
}

void Controller::updateOutputs()
{
    if (mode_ == MODE_WARNING)
    {
        fan_.setSpeed(100U);
        led_.setWarning();
        return;
    }

    if (mode_ == MODE_MANUAL)
    {
        led_.setManual();
        if (uartManualOverride_)
        {
            fan_.setSpeed(uartManualSpeed_);
        }
        else
        {
            fan_.setSpeed((uint8_t)(((uint32_t)potRaw_ * 100U) / 4095U));
        }
        return;
    }

    led_.setAuto();
    if (tempRaw_ <= TEMP_LOW_MAX)
        fan_.setSpeed(30U);
    else if (tempRaw_ <= TEMP_MID_MAX)
        fan_.setSpeed(60U);
    else
        fan_.setSpeed(100U);
}

void Controller::sendState()
{
    uart_.sendText("STATE mode=");
    if (mode_ == MODE_MANUAL)
        uart_.sendText("MANUAL");
    else if (mode_ == MODE_AUTO)
        uart_.sendText("AUTO");
    else
        uart_.sendText("WARNING");

    uart_.sendText(" fan=");
    uart_.sendUInt(fan_.speed());
    uart_.sendUInt(TIM2->CCR1);
    uart_.sendText(" pot=");
    uart_.sendUInt(((uint32_t)potRaw_ * 100U) / 4095U);
    uart_.sendText(" temp=");
    uart_.sendUInt(tempRaw_);
    uart_.sendText(" override=");
    uart_.sendUInt(uartManualOverride_ ? 1U : 0U);
    uart_.sendText("\r\n");
}