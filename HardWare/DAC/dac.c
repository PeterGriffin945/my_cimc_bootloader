#include "dac.h"

//配置了DAC0的输出引脚为PA4，使能DAC0的时钟，使能GPIOA的时钟，使能DAC0的时钟
void dac_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_DAC);

    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);

    dac_deinit(DAC0);

    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_SOFTWARE);
    dac_trigger_enable(DAC0, DAC_OUT0);

    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_DISABLE);

    dac_output_buffer_enable(DAC0, DAC_OUT0);

    dac_enable(DAC0, DAC_OUT0);
}

void dac_set_value(float voltage)
{
    uint16_t dac_value;

    if (voltage < 0.0f) {
        voltage = 0.0f;
    } else if (voltage > 3.3f) {
        voltage = 3.3f;
    }

    dac_value = (uint16_t)(voltage * 4095.0f / 3.3f);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, dac_value);
    dac_software_trigger_enable(DAC0, DAC_OUT0);
}
