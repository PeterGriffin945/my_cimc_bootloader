#include "sd_utils.h"
#include "diskio.h"
#include "sdcard.h"
#include <string.h>
#include <stdio.h>

FATFS fs;  // 在这里定义

void sd_utils_init(void)
{
		nvic_priority_group_set(NVIC_PRIGROUP_PRE1_SUB3);  // 必须在 sd_utils_init 之前
    nvic_irq_enable(SDIO_IRQn, 0, 0);
	
    DSTATUS stat;
    uint8_t retry = 5;
    do {
        stat = disk_initialize(0);
    } while ((stat != 0) && (--retry));

    printf("disk_initialize: %d\r\n", stat);

    if (stat == 0) {
        f_mount(0, &fs);
        printf("SD Card Init OK\r\n");
    } else {
        printf("SD Card Init Failed\r\n");
    }
}

void sd_log(const char *filename, const char *data)
{
    FIL  file;
    UINT bw;

    if (FR_OK != f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE)) return;
    f_lseek(&file, f_size(&file));   // 移到末尾
    f_write(&file, data, strlen(data), &bw);
    f_close(&file);
}
