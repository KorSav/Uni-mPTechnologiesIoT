#include <app.hpp>

volatile uint32_t g_ms = 0;
Uart1 g_uart;
AdcReader g_adc;

int main()
{
    __enable_irq();
    initGpio();
    initSysTickTimebase();
    g_uart.init();

    g_adc.init();
    g_adc.onStartConversion();

    const uint16_t psc = 7U;
    const uint16_t arr = 999U;
    initTim2FanPwm(psc, arr);
    initTim3LedPwm(psc, arr);

    PwmChannel fanPwm(&TIM2->CCR2, arr);
    PwmChannel ledPwm(&TIM3->CCR1, arr);
    Fan fan(fanPwm);
    StatusLed led(ledPwm);
    Button modeBtn(GPIOB, GPIO_IDR_IDR0);
    Button ackBtn(GPIOB, GPIO_IDR_IDR1);
    Controller controller(fan, led, g_uart);

    while (!g_adc.update())
        __NOP();
    controller.init(g_adc.getCh1(), g_adc.getCh2());

    uint32_t lastFast = millis();
    uint32_t lastAdc = millis();
    uint32_t lastPeriodic = millis();

    while (1)
    {
        uint32_t now = millis();

        if ((now - lastFast) >= 10U)
        {
            lastFast = now;
            controller.onFastTick();
        }

        if ((now - lastAdc) >= 100U)
        {
            lastAdc = now;
            g_adc.onStartConversion();
        }

        if ((now - lastPeriodic) >= 1000U)
        {
            lastPeriodic = now;
            controller.onPeriodicTick();
        }

        if (g_adc.update())
            controller.onAdcSample(g_adc.getCh1(), g_adc.getCh2());

        if (modeBtn.update())
            controller.onModeButton();

        if (ackBtn.update())
            controller.onAckButton();

        if (g_uart.consumeDroppedLineFlag())
            controller.onUartOverflow();

        if (g_uart.hasLine())
            controller.onUartLine();

        __WFI();
    }
}
