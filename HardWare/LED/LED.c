/*
 * LED驱动, 6个LED接在PA4~PA9
 * 作者: Lingyu Meng   2025/3/15
 */

#include "LED.h"

void LED_Init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                  GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                  GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                  GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    gpio_bit_reset(GPIOA, GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 |
                          GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
}

void LED_Control(LED_Name_TypeDef led, LED_State_TypeDef state)
{
    uint32_t pin;

    switch (led) {
        case LED_1: pin = LED_PIN_1; break;
        case LED_2: pin = LED_PIN_2; break;
        case LED_3: pin = LED_PIN_3; break;
        case LED_4: pin = LED_PIN_4; break;
        case LED_5: pin = LED_PIN_5; break;
        case LED_6: pin = LED_PIN_6; break;
        default: return;
    }

    if (state == LED_ON)
        gpio_bit_set(GPIOA, pin);
    else
        gpio_bit_reset(GPIOA, pin);
}
