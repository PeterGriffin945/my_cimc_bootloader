//使用rs485通信，满足低功耗需求，接到了usart1，传输速率19200，pd5是tx，ps6是rx，pe8控制收发

#include "usart.h"

unsigned char rx_buf[BUF_SIZE];
unsigned int  rx_len = 0;
unsigned char rx_done = 0;

void usart_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_USART1);

    // PD5=TX, PD6=RX, AF7
    gpio_af_set(GPIOD, GPIO_AF_7, GPIO_PIN_5 | GPIO_PIN_6);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_5);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_mode_set(GPIOD, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_6);
    gpio_output_options_set(GPIOD, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);

    // PE8=485方向, 默认接收
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_8);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);
    gpio_bit_reset(GPIOE, GPIO_PIN_8);

    usart_deinit(USART1);
    usart_baudrate_set(USART1, 19200U);
    usart_word_length_set(USART1, USART_WL_8BIT);
    usart_stop_bit_set(USART1, USART_STB_1BIT);
    usart_parity_config(USART1, USART_PM_NONE);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);

    // RBNE中断(收字节) + IDLE中断(帧结束)
    nvic_irq_enable(USART1_IRQn, 3, 0);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
    usart_interrupt_enable(USART1, USART_INT_IDLE);

    usart_enable(USART1);
}

// 重写printf函数
int fputc(int ch, FILE *f)
{
    gpio_bit_set(GPIOE, GPIO_PIN_8);

    usart_data_transmit(USART1, (unsigned char)ch);
    while (!usart_flag_get(USART1, USART_FLAG_TC));
    usart_flag_clear(USART1, USART_FLAG_TC);

    gpio_bit_reset(GPIOE, GPIO_PIN_8);

    return ch;
}

void USART1_IRQHandler(void)
{
    // 收字节
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE) != RESET) {
        unsigned char data = usart_data_receive(USART1);
        if (rx_len < BUF_SIZE) {
            rx_buf[rx_len] = data;
            rx_len++;
        }
    }

    // IDLE: 一帧完了
    if (usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE) != RESET) {
        volatile unsigned int tmp;
        tmp = USART_STAT0(USART1);
        tmp = USART_DATA(USART1);
        (void)tmp;

        if (rx_len > 0) {
            rx_done = 1;
            usart_interrupt_disable(USART1, USART_INT_RBNE);
            usart_interrupt_disable(USART1, USART_INT_IDLE);
        }
    }
}
