#include "Function.h"
#include "bootloader_core.h"

void System_Init(void)
{
    systick_config();
    usart_init();
}

void UsrFunction(void)
{
    bootloader_run();
    while (1);
}
