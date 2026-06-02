/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：led.h
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/
#ifndef __LED_H
#define __LED_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/

// LED引脚定义
#define LED_PIN_1   GPIO_PIN_4
#define LED_PIN_2   GPIO_PIN_5
#define LED_PIN_3   GPIO_PIN_6
#define LED_PIN_4   GPIO_PIN_7
#define LED_PIN_5   GPIO_PIN_8
#define LED_PIN_6   GPIO_PIN_9

/************************ 枚举定义 ************************/

typedef enum {
    LED_1 = 0,
    LED_2 = 1,
    LED_3 = 2,
    LED_4 = 3,
    LED_5 = 4,
    LED_6 = 5
} LED_Name_TypeDef;

typedef enum {
    LED_OFF = 0,
    LED_ON = 1
} LED_State_TypeDef;

/************************ 变量定义 ************************/


/************************ 函数定义 ************************/

void LED_Init(void);
void LED_Control(LED_Name_TypeDef led, LED_State_TypeDef state);

#endif


/****************************End*****************************/

