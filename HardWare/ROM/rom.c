//一页flash是4k，使用之前要先执行擦命令！

#include "rom.h"

// 擦一页
void flash_erase(uint32_t addr)
{
    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_page_erase(addr);
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGSERR | FMC_FLAG_PGMERR);
    fmc_lock();
}

// 按字节写, len是字节数
void flash_write(uint32_t addr, uint8_t *buf, uint32_t len)
{
    fmc_unlock();
    for (uint32_t i = 0; i < len; i++) {
        fmc_byte_program(addr + i, buf[i]);
        while (fmc_flag_get(FMC_FLAG_BUSY));  // 等写入完成
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
