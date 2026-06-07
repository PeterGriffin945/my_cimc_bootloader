#include "bootloader_core.h"
#include "gd32f4xx.h"
#include "usart.h"
#include "rom.h"
#include <stdio.h>
#include <string.h>

/* 内存分区 */
#define PARA_BASE       0x08010000   /* 参数区基址 */
#define APP_ADDR        0x08011000   /* 应用区 */
#define STAGING_ADDR    0x08051000   /* 固件暂存区 */
#define STAGING_SIZE    (128UL * 1024UL)

#define SLICE_SIZE      256           /* 固件切片大小 */

/* 协议常量 */
#define PRO_VER         0x02
#define T_CMD           0x01          /* 命令帧 */
#define T_ACK           0x02          /* 应答帧 */
#define T_ERR           0xFF          /* 错误帧 */
#define T_HEART         0x05          /* 心跳帧 */
#define C_PREP          0x0502        /* 准备传输固件 */
#define C_EXEC          0x0503        /* 执行升级 */
#define C_FIND          0xFFFF        /* 广播寻找 */
#define C_HEARTBEAT     0x8888        /* 心跳回复 */
#define C_ERRUNKN       0xEEEE        /* 未知命令错误 */

/* USART 中断接收变量, 在 usart.c 定义 */
extern unsigned char rx_buf[];
extern unsigned int  rx_len;
extern unsigned char rx_done;

/* ===== CRC-16 Modbus ===== */
static const uint16_t crc_tab[256] = {
    0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
    0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
    0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
    0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
    0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
    0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
    0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
    0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
    0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
    0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
    0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
    0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
    0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
    0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
    0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
    0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
    0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
    0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
    0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
    0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
    0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
    0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
    0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
    0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
    0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
    0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
    0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
    0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
    0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
    0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
    0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
    0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040,
};

static uint16_t cal_crc16(uint8_t *d, uint32_t n)
{
    uint16_t crc = 0xFFFF;
    for (uint32_t i = 0; i < n; i++)
        crc = (crc >> 8) ^ crc_tab[(crc ^ d[i]) & 0xFF];
    return crc;
}

/* ===== 发送 ===== */

/* 发送原始 ASCII 字符串 (不经帧封装) */
static void rs485_send_str(const char *s)
{
    gpio_bit_set(GPIOE, GPIO_PIN_8);
    while (*s) {
        usart_data_transmit(USART1, (uint8_t)(*s++));
        while (!usart_flag_get(USART1, USART_FLAG_TC));
        usart_flag_clear(USART1, USART_FLAG_TC);
    }
    gpio_bit_reset(GPIOE, GPIO_PIN_8);
}

/* 发送协议帧 (ASCII 十六进制编码) */
static void send_frame(uint8_t type, uint16_t cmd, uint8_t *data, uint8_t len)
{
    uint8_t raw[260];
    uint16_t i, idx = 0;

    raw[idx++] = 0xA5; raw[idx++] = 0xB6;
    raw[idx++] = 0;    raw[idx++] = 0;   /* 设备ID (暂填0) */
    raw[idx++] = type;
    raw[idx++] = cmd >> 8;   raw[idx++] = cmd & 0xFF;
    raw[idx++] = len;
    raw[idx++] = PRO_VER;
    for (i = 0; i < len; i++) raw[idx++] = data[i];

    uint16_t crc = cal_crc16(raw, idx);
    raw[idx++] = crc >> 8;    raw[idx++] = crc & 0xFF;
    raw[idx++] = 0xB6;        raw[idx++] = 0xA5;

    /* ASCII 十六进制编码 */
    uint8_t ascii[520];
    for (i = 0; i < idx; i++) {
        uint8_t h = raw[i] >> 4, l = raw[i] & 0x0F;
        ascii[i*2]   = (h > 9) ? (h - 10 + 'A') : (h + '0');
        ascii[i*2+1] = (l > 9) ? (l - 10 + 'A') : (l + '0');
    }

    gpio_bit_set(GPIOE, GPIO_PIN_8);
    for (i = 0; i < idx * 2; i++) {
        usart_data_transmit(USART1, ascii[i]);
        while (!usart_flag_get(USART1, USART_FLAG_TC));
        usart_flag_clear(USART1, USART_FLAG_TC);
    }
    gpio_bit_reset(GPIOE, GPIO_PIN_8);
}

/* ===== 接收 ===== */

/* 等待一帧 ASCII 十六进制数据, 超时返回 0 */
static int wait_frame(uint32_t timeout_ms)
{
    rx_len = 0;
    rx_done = 0;
    usart_interrupt_enable(USART1, USART_INT_RBNE);
    usart_interrupt_enable(USART1, USART_INT_IDLE);

    for (uint32_t i = 0; i < timeout_ms; i++) {
        delay_1ms(1);
        if (rx_done) {
            usart_interrupt_disable(USART1, USART_INT_RBNE);
            usart_interrupt_disable(USART1, USART_INT_IDLE);
            return 1;
        }
    }
    usart_interrupt_disable(USART1, USART_INT_RBNE);
    usart_interrupt_disable(USART1, USART_INT_IDLE);
    return 0;
}

/* 解析 ASCII 十六进制帧, 提取类型/命令/内容 */
static int parse_frame(uint8_t *type, uint16_t *cmd, uint8_t *cont, uint8_t *clen)
{
    uint8_t raw[260];
    uint16_t i;

    raw[0] = 0xA5; raw[1] = 0xB6;
    for (i = 0; i < rx_len; i += 2) {
        uint8_t h = rx_buf[i], l = rx_buf[i+1];
        h = (h >= 'A') ? (h - 'A' + 10) : (h - '0');
        l = (l >= 'A') ? (l - 'A' + 10) : (l - '0');
        raw[i/2 + 2] = (h << 4) | l;
    }
    uint16_t rlen = rx_len / 2 + 2;   /* 含手动填充的 A5B6 */

    if (rlen < 13) return 0;           /* 最短帧: hdr+id+type+cmd+len+ver+crc+tail */
    if (raw[0] != 0xA5 || raw[1] != 0xB6)
        return 0;
    if (raw[7] > 250) return 0;        /* 内容长度不合理 */

    uint8_t n = raw[7];
    if (rlen != 13 + n) return 0;      /* 总长校验 */

    uint16_t crc_len = 9 + n;
    uint16_t calc = cal_crc16(raw, crc_len);
    uint16_t frm_crc = (raw[crc_len] << 8) | raw[crc_len + 1];
    if (calc != frm_crc) return 0;

    *type = raw[4];
    *cmd  = (raw[5] << 8) | raw[6];
    *clen = n;
    for (i = 0; i < n; i++) cont[i] = raw[9 + i];
    return 1;
}

/* 在指定毫秒内监听 0x0502, 同时处理广播查找帧 */
static int wait_for_prep(uint32_t ms, uint16_t dev_id)
{
    uint8_t type, clen;
    uint16_t cmd;
    uint8_t cont[260];
    uint32_t elapsed = 0;

    while (elapsed < ms) {
        uint32_t chunk = (ms - elapsed > 100) ? 100 : (ms - elapsed);
        if (wait_frame(chunk)) {
            if (parse_frame(&type, &cmd, cont, &clen)) {
                if (cmd == C_PREP && type == T_CMD)
                    return 1;   /* 收到 0x0502, 中断倒计时 */
                if (cmd == C_FIND && type == T_HEART) {
                    uint8_t idb[2];
                    idb[0] = dev_id >> 8; idb[1] = dev_id & 0xFF;
                    send_frame(T_HEART, C_HEARTBEAT, idb, 2);
                } else {
                    send_frame(T_ERR, C_ERRUNKN, NULL, 0);
                }
            } else {
                send_frame(T_ERR, C_ERRUNKN, NULL, 0);
            }
        }
        elapsed += chunk;
    }
    return 0;
}

/* ===== 固件接收 ===== */

/* 从串口接收原始字节到暂存区, 返回固件总字节数 */
static uint32_t recv_firmware(void)
{
    uint8_t slice[SLICE_SIZE];
    uint32_t slen = 0, total = 0, tout = 0;
    int got_data = 0;

    /* 擦除暂存区 (32 页 × 4K) */
    for (int i = 0; i < 32; i++)
        flash_erase(STAGING_ADDR + i * 4096);

    /* 关 IRQ, 直接轮询 RBNE 收原始字节 */
    NVIC_DisableIRQ(USART1_IRQn);

    while (total < STAGING_SIZE) {
        if (usart_flag_get(USART1, USART_FLAG_RBNE)) {
            slice[slen++] = usart_data_receive(USART1);
            total++;
            got_data = 1;
            tout = 0;

            if (slen == SLICE_SIZE) {
                flash_write(STAGING_ADDR + total - SLICE_SIZE, slice, SLICE_SIZE);
                slen = 0;
            }
        } else {
            delay_1ms(1);
            tout++;
            if (got_data && tout > 500)  break;
            if (!got_data && tout > 3000) break;
        }
    }

    NVIC_EnableIRQ(USART1_IRQn);

    /* 清除收固件残留的 IDLE 标志, 避免污染后续帧接收 */
    if (usart_flag_get(USART1, USART_FLAG_IDLE)) {
        volatile uint32_t tmp = USART_STAT0(USART1);
        tmp = USART_DATA(USART1);
        (void)tmp;
    }

    /* 写剩余不足切片的数据 */
    if (slen > 0)
        flash_write(STAGING_ADDR + total - slen, slice, slen);

    return total;
}

/* ===== 跳转 ===== */

static void led2_toggle(void)
{
    static int init = 0;
    if (!init) {
        rcu_periph_clock_enable(RCU_GPIOA);
        gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_5);
        gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
        gpio_bit_reset(GPIOA, GPIO_PIN_5);
        init = 1;
    }
    gpio_bit_toggle(GPIOA, GPIO_PIN_5);
}

static void jump_to_app(void)
{
    /* 闪 3 下 LED2 再跳, 直观确认跳前没卡住 */
    for (int i = 0; i < 6; i++) {
        led2_toggle();
        delay_1ms(100);
    }
    gpio_bit_reset(GPIOA, GPIO_PIN_5);

    if (*(volatile uint32_t *)APP_ADDR == 0xFFFFFFFF) {
        while (1) { led2_toggle(); delay_1ms(200); }
    }
    if ((*(volatile uint32_t *)(APP_ADDR + 4) & 0xFF000000) != 0x08000000) {
        while (1) { led2_toggle(); delay_1ms(100); }
    }

    __disable_irq();
    SCB->VTOR = APP_ADDR;
    __set_MSP(*(volatile uint32_t *)APP_ADDR);
    ((void(*)(void))(*(volatile uint32_t *)(APP_ADDR + 4)))();
}

/* ===== 主流程 ===== */

/* APP 参数区 (0x08031000) 波特率扫描 */
#define APP_PARA_BASE   0x08031000
#define APP_PARA_SZ     64
#define APP_PARA_CNT    (4096 / 64)

static uint8_t get_baud_from_app_para(void)
{
    for (int i = APP_PARA_CNT - 1; i >= 0; i--) {
        uint32_t *p = (uint32_t *)(APP_PARA_BASE + i * APP_PARA_SZ);
        if (*p == 0xA5A5A5A5) {
            uint8_t *rec = (uint8_t *)p;
            return rec[22];  /* rec_t.baud offset */
        }
    }
    return 0xFF;  /* 未找到 */
}

void bootloader_run(void)
{
    uint8_t type, clen;
    uint16_t cmd;
    uint8_t cont[260];
    uint32_t fw_len;

    /* 从 APP param_store 读取已保存的波特率，覆盖 usart_init 的 19200 默认值 */
    {
        uint8_t bc = get_baud_from_app_para();
        uint32_t tbl[] = {4800, 9600, 19200, 115200};
        if (bc >= 0x11 && bc <= 0x14)
            usart_set_baud(tbl[bc - 0x11]);
    }

    uint32_t flag = *(volatile uint32_t *)PARA_BASE;

    if (flag == 0xAABBCCDD) {
        /* ---- 升级模式 ---- */

        /* 读设备 ID, 若未设置则用 0x0001 */
        uint16_t dev_id = *(volatile uint16_t *)(PARA_BASE + 8);
        if (dev_id == 0xFFFF) dev_id = 0x0001;

        /* 擦除参数区 (清除升级标志), 回写设备 ID */
        fmc_unlock();
        fmc_page_erase(PARA_BASE);
        while (fmc_flag_get(FMC_FLAG_BUSY));
        fmc_word_program(PARA_BASE + 8, dev_id);
        while (fmc_flag_get(FMC_FLAG_BUSY));
        fmc_lock();

        /* 倒计时并监听 0x0502 */
        rs485_send_str("using command to interrupt start Application\r\n");

        rs485_send_str("wait for start Application(10s)……\r\n");
        if (wait_for_prep(3000, dev_id)) goto recv_fw;

        rs485_send_str("wait for start Application(7s)……\r\n");
        if (wait_for_prep(3000, dev_id)) goto recv_fw;

        rs485_send_str("wait for start Application(4s)……\r\n");
        if (wait_for_prep(3000, dev_id)) goto recv_fw;

        rs485_send_str("wait for start Application(1s)……\r\n");
        if (wait_for_prep(1000, dev_id)) goto recv_fw;

        /* 10 秒到, 跳 APP */
        jump_to_app();

    recv_fw:
        /* 回复 0x0502 ACK */
        {
            uint8_t ok = 0xFF;
            send_frame(T_ACK, C_PREP, &ok, 1);
        }

        /* 收固件 */
        fw_len = recv_firmware();

        /* 校验魔术字 (前4字节: 5A A5 C3 3C) */
        {
            int magic_ok = 0;
            if (fw_len >= 4) {
                uint8_t m[4];
                for (int i = 0; i < 4; i++)
                    m[i] = *(volatile uint8_t *)(STAGING_ADDR + i);
                if (m[0] == 0x5A && m[1] == 0xA5 && m[2] == 0xC3 && m[3] == 0x3C)
                    magic_ok = 1;
            }
            if (!magic_ok) {
                send_frame(T_ERR, C_PREP, NULL, 0);
                delay_1ms(500);
                __set_FAULTMASK(1);
                NVIC_SystemReset();
            }
        }

        {
            uint8_t ok = 0xFF;
            send_frame(T_ACK, C_PREP, &ok, 1);
        }

        /* 等 0x0503 (最多 30 秒) */
        {
            uint32_t i;
            for (i = 0; i < 300; i++) {
                if (wait_frame(100)) {
                    if (parse_frame(&type, &cmd, cont, &clen) && cmd == C_EXEC)
                        break;
                    send_frame(T_ERR, C_ERRUNKN, NULL, 0);
                }
            }
            if (i >= 300) {
                __set_FAULTMASK(1);
                NVIC_SystemReset();
            }
        }

        {
            uint8_t ok = 0xFF;
            send_frame(T_ACK, C_EXEC, &ok, 1);
        }

        /* 搬运固件: 擦 APP 区 → 读暂存区 → 写 APP 区 */
        {
            unsigned int pages = (fw_len + 4095) / 4096;
            for (unsigned int i = 0; i < pages; i++)
                flash_erase(APP_ADDR + i * 4096);

            uint8_t tmp[256];
            unsigned int off = 0;
            while (off < fw_len) {
                unsigned int chunk = (fw_len - off > 256) ? 256 : (fw_len - off);
                for (unsigned int j = 0; j < chunk; j++)
                    tmp[j] = *(volatile uint8_t *)(STAGING_ADDR + off + j);
                flash_write(APP_ADDR + off, tmp, chunk);
                off += chunk;
            }
        }

        /* 跳 APP */
        jump_to_app();
    }

    /* ---- 普通启动: LED2 慢闪 5 秒, 然后跳 APP ---- */
    for (int i = 0; i < 10; i++) {
        led2_toggle();
        delay_1ms(500);
    }
    jump_to_app();
}
