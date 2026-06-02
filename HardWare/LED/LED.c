/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：led.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/

/************************* 头文件 *************************/

#include "LED.h"

/************************ 全局变量定义 ************************/


/************************************************************ 
 * Function :       LED_Init
 * Comment  :       用于初始化LED端口
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-02-30 V0.1 original
************************************************************/

void LED_Init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);
	
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9);
	gpio_bit_reset(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9);
}

void LED_Control(LED_Name_TypeDef led, LED_State_TypeDef state)
{
	uint32_t led_pin;
	
	switch(led) {
		case LED_1: led_pin = LED_PIN_1; break;
		case LED_2: led_pin = LED_PIN_2; break;
		case LED_3: led_pin = LED_PIN_3; break;
		case LED_4: led_pin = LED_PIN_4; break;
		case LED_5: led_pin = LED_PIN_5; break;
		case LED_6: led_pin = LED_PIN_6; break;
		default: return;
	}
	
	if (state == LED_ON) {
		gpio_bit_set(GPIOA, led_pin);
	} else {
		gpio_bit_reset(GPIOA, led_pin);
	}
}



/****************************End*****************************/

