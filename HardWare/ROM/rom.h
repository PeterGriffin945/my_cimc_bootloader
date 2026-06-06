#ifndef __ROM_H__
#define __ROM_H__

#include "gd32f4xx.h"

void     flash_erase(uint32_t addr);
void     flash_write(uint32_t addr, uint8_t *buf, uint32_t len);
uint8_t  flash_read8(uint32_t addr);
uint32_t flash_read32(uint32_t addr);

#endif
