/*
 * LED.h
 * 作者: Lingyu Meng   2025/3/15
 */

#ifndef __LED_H
#define __LED_H

#include "HeaderFiles.h"

#define LED_PIN_1   GPIO_PIN_4
#define LED_PIN_2   GPIO_PIN_5
#define LED_PIN_3   GPIO_PIN_6
#define LED_PIN_4   GPIO_PIN_7
#define LED_PIN_5   GPIO_PIN_8
#define LED_PIN_6   GPIO_PIN_9

typedef enum {
    LED_1 = 0,
    LED_2,
    LED_3,
    LED_4,
    LED_5,
    LED_6
} LED_Name_TypeDef;

typedef enum {
    LED_OFF = 0,
    LED_ON
} LED_State_TypeDef;

void LED_Init(void);
void LED_Control(LED_Name_TypeDef led, LED_State_TypeDef state);

#endif
