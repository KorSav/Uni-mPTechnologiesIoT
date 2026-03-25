#include <app.hpp>

extern "C" void ADC1_2_IRQHandler()
{
    g_adc.onAdcIrq();
}

void AdcReader::init()
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->CR1 |= ADC_CR1_EOCIE;
    ADC1->CR2 |= ADC_CR2_EXTTRIG;
    ADC1->CR2 |= ADC_CR2_EXTSEL;

    ADC1->SMPR2 |= ADC_SMPR2_SMP2;
    ADC1->SMPR2 |= ADC_SMPR2_SMP3;

    ADC1->CR2 |= ADC_CR2_ADON;
    NVIC_EnableIRQ(ADC1_2_IRQn);
}

uint16_t AdcReader::getCh1() const
{
    return ch1_;
}

uint16_t AdcReader::getCh2() const
{
    return ch2_;
}

bool AdcReader::update()
{
    bool res;
    if (conversionBusy_)
    {
        res = false;
    }
    else
    {
        res = dataChanged_;
        dataChanged_ = false;
    }
    return res;
}

void AdcReader::onStartConversion()
{
    if (conversionBusy_)
        return;

    ADC1->SR = 0;
    conversionBusy_ = true;
    dataChanged_ = false;
    aggChange_ = false;

    ADC1->SQR3 = 2U << ADC_SQR3_SQ1_Pos;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void AdcReader::onAdcIrq()
{
    if ((ADC1->SR & ADC_SR_EOC) == 0U)
        return;

    const uint16_t sample = static_cast<uint16_t>(ADC1->DR);

    if (seqIndex_ == 0U)
    {
        latestCh1Raw_ = sample;
        aggChange_ = aggChange_ || (ch1_ != sample);
        ADC1->SQR3 = 3U << ADC_SQR3_SQ1_Pos;
        ADC1->CR2 |= ADC_CR2_SWSTART;
    }
    else
    {
        latestCh2Raw_ = sample;
        aggChange_ = aggChange_ || (ch2_ != sample);
        ch1_ = latestCh1Raw_;
        ch2_ = latestCh2Raw_;
        conversionBusy_ = false;
        dataChanged_ = aggChange_;
    }

    seqIndex_ = (seqIndex_ + 1) % 2; // amount of channels
}
