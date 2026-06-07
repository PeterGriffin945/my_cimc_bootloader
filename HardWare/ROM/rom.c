//一页flash是4k，使用之前要先执行擦命令！

#include "rom.h"
#include <string.h>

/* 地址 → 扇区号 (GD32F470VE 3MB Flash 布局) */
static uint32_t addr2sector(uint32_t addr)
{
    if (addr < 0x08004000) return 0;
    if (addr < 0x08008000) return 1;
    if (addr < 0x0800C000) return 2;
    if (addr < 0x08010000) return 3;
    if (addr < 0x08020000) return 4;
    /* 从 sector 5 起全部 128KB */
    uint32_t sec = 5 + ((addr - 0x08020000) / 0x20000);
    if (sec > 27) sec = 27;
    return sec;
}

/* 擦除整扇区 */
void flash_erase(uint32_t addr)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_sector_erase(CTL_SN(addr2sector(addr)));
    while (fmc_flag_get(FMC_FLAG_BUSY));
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_lock();
}

/* 按字写入 (GD32F4 FMC 不支持 byte program, 必须用 word) */
void flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);

    uint32_t i = 0;
    for (; i + 4 <= len; i += 4) {
        uint32_t word;
        memcpy(&word, buf + i, 4);
        fmc_word_program(addr + i, word);
        while (fmc_flag_get(FMC_FLAG_BUSY));
    }
    /* 末尾不足 4 字节, 补 0xFF 凑成 word 写入 */
    if (i < len) {
        uint32_t word = 0xFFFFFFFF;
        memcpy(&word, buf + i, len - i);
        fmc_word_program(addr + i, word);
        while (fmc_flag_get(FMC_FLAG_BUSY));
    }

    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_lock();
}

// Flash内存映射的, 直接指针读就行
uint8_t flash_read8(uint32_t addr)
{
    return *(volatile uint8_t *)addr;
}

uint32_t flash_read32(uint32_t addr)
{
    return *(volatile uint32_t *)addr;
}
