#include <app.hpp>

extern "C" void USART1_IRQHandler()
{
    g_uart.irqHandler();
}

void Uart1::init()
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1->BRR = 0x0341; // 9600 baud for 8 MHz PCLK2
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;
    NVIC_EnableIRQ(USART1_IRQn);
}

void Uart1::irqHandler()
{
    if ((USART1->SR & USART_SR_RXNE) == 0U)
        return;

    char c = (char)USART1->DR;

    if (c == '\r' || c == '\n')
    {
        if (overflow_)
        {
            overflow_ = false;
            rxLen_ = 0;
            rxBuf_[0] = '\0';
            lineReady_ = false;
            lineDropped_ = true;
            return;
        }

        if (rxLen_ > 0U && !lineReady_)
        {
            rxBuf_[rxLen_] = '\0';
            lineReady_ = true;
        }
        rxLen_ = 0;
        return;
    }

    if (lineReady_)
        return;

    if (rxLen_ < sizeof(rxBuf_) - 1U)
    {
        rxBuf_[rxLen_++] = c;
    }
    else
    {
        rxLen_ = 0;
        overflow_ = true;
    }
}

bool Uart1::hasLine() const
{
    return lineReady_;
}

bool Uart1::consumeDroppedLineFlag()
{
    bool v = lineDropped_;
    lineDropped_ = false;
    return v;
}

void Uart1::readLine(char *out, uint32_t maxLen)
{
    if (maxLen == 0U)
        return;

    __disable_irq();
    uint32_t i = 0U;
    while ((i + 1U) < maxLen && rxBuf_[i] != '\0')
    {
        out[i] = rxBuf_[i];
        ++i;
    }
    out[i] = '\0';
    lineReady_ = false;
    __enable_irq();
}

void Uart1::sendChar(char c)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }
    USART1->DR = (uint16_t)c;
}

void Uart1::sendText(const char *s)
{
    while (*s)
        sendChar(*s++);
}

void Uart1::sendLine(const char *s)
{
    sendText(s);
    sendText("\r\n");
}

void Uart1::sendUInt(uint32_t value)
{
    char buf[11];
    utoa10(value, buf);
    sendText(buf);
}