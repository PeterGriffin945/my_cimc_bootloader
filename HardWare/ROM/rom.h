#ifndef __ROM_H__
#define __ROM_H__

#include "gd32f4xx.h"

void     internal_flash_erase(uint32_t addr);
void     internal_flash_write_str_Char(uint32_t addr, uint8_t *data, uint32_t len);
uint8_t  internal_flash_read_Char(uint32_t addr);
uint32_t internal_flash_read_word(uint32_t addr);

#endif
