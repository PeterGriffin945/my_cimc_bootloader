#include "rom.h"

/* 擦除一页 (4KB) */
void internal_flash_erase(uint32_t addr)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(addr);
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_lock();
}

/* 按字节写入，len 是字节数 */
void internal_flash_write_str_Char(uint32_t addr, uint8_t *data, uint32_t len)
{
    fmc_unlock();
    for (uint32_t i = 0; i < len; i++) {
        fmc_byte_program(addr + i, data[i]);
        while (fmc_flag_get(FMC_FLAG_BUSY));
    }
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_lock();
}

/* 读一个字节 (Flash 是内存映射的，直接指针读) */
uint8_t internal_flash_read_Char(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

/* 读一个字 (32位) */
uint32_t internal_flash_read_word(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}
