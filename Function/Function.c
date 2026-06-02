#include "Function.h"
#include <string.h>
#include "rom.h"

extern unsigned char rx_buf[];
extern unsigned int  rx_len;
extern unsigned char rx_done;

#define APP_ADDR  0x0800D000

typedef void (*func)(void);

void mcu_software_reset(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

// 跳到 App 地址
void iap_load_app(unsigned int addr)
{
    // 检查栈指针是否在 SRAM 范围
    if (((*(unsigned int *)addr) & 0x2FFE0000) != 0x20000000)
        return;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    for (int i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    __DSB();
    __ISB();

    SCB->VTOR = addr;

    __set_MSP(*(unsigned int *)addr);

    __enable_irq();

    ((func)(*(unsigned int *)(addr + 4)))();
}

void jump_to_app(void)
{
    unsigned int entry = *(unsigned int *)(APP_ADDR + 4);

    // 入口地址必须在 Flash 范围
    if ((entry & 0xFF000000) == 0x08000000)
        iap_load_app(APP_ADDR);
    else
        while (1);
}

void System_Init(void)
{
    systick_config();
    usart_init();
    LED_Init();
}

void UsrFunction(void)
{
    unsigned int i, pages;
    unsigned int wait;

    printf("\r\n===== BootLoader =====\r\n");

    // 等 3 秒，每秒提示一次
    for (wait = 3; wait > 0; wait--)
    {
        printf("send 'update' in %ds if you want to update\r\n", wait);

        for (i = 0; i < 1000; i++)
        {
            delay_1ms(1);
            if (rx_done)
            {
                rx_done = 0;
                if (strstr((char *)rx_buf, "update"))
                    goto recv_fw;

                // 不是 update，继续等
                rx_len = 0;
                usart_interrupt_enable(USART1, USART_INT_RBNE);
                usart_interrupt_enable(USART1, USART_INT_IDLE);
            }
        }
    }

    printf("timeout, jump to app...\r\n");
    delay_1ms(100);
    jump_to_app();
    while (1);

recv_fw:
    printf("enter update mode, send firmware...\r\n");

    rx_len = 0;
    rx_done = 0;
    // 只开 RBNE 中断收字节，不用 IDLE
    usart_interrupt_enable(USART1, USART_INT_RBNE);

    // 用超时判断收完了没有（500ms 没新字节就算完）
    {
        unsigned int timeout = 0;
        unsigned int prev_len = 0;
        while (1)
        {
            delay_1ms(1);
            if (rx_len > prev_len)
            {
                prev_len = rx_len;
                timeout = 0;
            }
            else
            {
                timeout++;
                if (timeout > 500 && rx_len > 0)
                    break;
            }
        }
    }

    printf("received %d bytes\r\n", rx_len);

    if (rx_len > BUF_SIZE)
    {
        printf("firmware too big!\r\n");
        mcu_software_reset();
    }

    // 擦 App 区
    pages = (rx_len + 4095) / 4096;
    printf("erasing %d pages...\r\n", pages);
    for (i = 0; i < pages; i++)
        internal_flash_erase(APP_ADDR + i * 4096);

    // 写入 Flash
    printf("writing...\r\n");
    internal_flash_write_str_Char(APP_ADDR, rx_buf, rx_len);

    // 读出验证前 64 字节
    printf("verify:\r\n");
    for (i = 0; i < 64 && i < rx_len; i++)
        printf("%02X ", internal_flash_read_Char(APP_ADDR + i));
    printf("\r\n");

    delay_1ms(500);
    printf("rebooting...\r\n");
    delay_1ms(100);
    mcu_software_reset();

    while (1);
}
