#ifndef SD_UTILS_H
#define SD_UTILS_H

#include "ff.h"
#include "HeaderFiles.h"

extern FATFS fs;  // 全局文件系统对象，在 sd_utils.c 里定义

void sd_utils_init(void);               // SD卡 + FatFS 初始化
void sd_log(const char *filename, const char *data);  // 追加写文件

#endif
