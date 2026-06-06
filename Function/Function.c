#include "Function.h"
#include "rom.h"

#define APP_ADDR      0x08011000
#define STAGING_ADDR  0x08051000
#define STAGING_SIZE  (128UL * 1024UL)

#define SLICE_SIZE    256

#define FW_MAGIC  0x5AA5C33C

#define PRO_VER  0x02
#define T_CMD    0x01
#define T_ACK    0x02
#define T_ERR    0xFF
#define C_PREP   0x0502
#define C_EXEC   0x0503

extern unsigned char rx_buf[];
extern unsigned int  rx_len;
extern unsigned char rx_done;

typedef void (*func)(void);

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

static void send_frame(uint8_t type, uint16_t cmd, uint8_t *data, uint8_t len)
{
    uint8_t raw[260];
    uint16_t i, idx = 0;

    raw[idx++] = 0xA5; raw[idx++] = 0xB6;
    raw[idx++] = 0;    raw[idx++] = 0;
    raw[idx++] = type;
    raw[idx++] = cmd >> 8;   raw[idx++] = cmd & 0xFF;
    raw[idx++] = len;
    raw[idx++] = PRO_VER;
    for (i = 0; i < len; i++) raw[idx++] = data[i];

    uint16_t crc = cal_crc16(raw, idx);
    raw[idx++] = crc >> 8;    raw[idx++] = crc & 0xFF;
    raw[idx++] = 0xB6;        raw[idx++] = 0xA5;

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

static void mcu_rst(void)
{
    __set_FAULTMASK(1);
    NVIC_SystemReset();
}

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

static int parse_frame(uint16_t *cmd)
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
    uint16_t rlen = rx_len / 2 + 2;

    if (rlen < 13) return 0;
    if (raw[0] != 0xA5 || raw[1] != 0xB6) return 0;

    uint8_t n = raw[7];
    if (rlen != 13 + n) return 0;

    uint16_t crc_len = 9 + n;
    uint16_t calc = cal_crc16(raw, crc_len);
    uint16_t frm_crc = (raw[crc_len] << 8) | raw[crc_len + 1];
    if (calc != frm_crc) return 0;

    *cmd = (raw[5] << 8) | raw[6];
    return 1;
}

static unsigned int recv_firmware(void)
{
    uint8_t slice[SLICE_SIZE];
    unsigned int slen = 0, total = 0, tout = 0;
    int got_data = 0;

    for (int i = 0; i < 32; i++)
        flash_erase(STAGING_ADDR + i * 4096);

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
            if (got_data && tout > 500)
                break;
            if (!got_data && tout > 3000)
                break;
        }
    }

    NVIC_EnableIRQ(USART1_IRQn);

    if (slen > 0)
        flash_write(STAGING_ADDR + total - slen, slice, slen);

    return total;
}

static void jump_to_app(void)
{
    unsigned int entry = *(unsigned int *)(APP_ADDR + 4);
    if ((entry & 0xFF000000) != 0x08000000)
        while (1);

    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    __DSB(); __ISB();
    SCB->VTOR = APP_ADDR;
    __set_MSP(*(unsigned int *)APP_ADDR);
    __enable_irq();
    ((func)(*(unsigned int *)(APP_ADDR + 4)))();
}

void System_Init(void)
{
    systick_config();
    usart_init();
}

void UsrFunction(void)
{
    uint32_t i;
    uint16_t cmd;

    /* 上电后读RTC_BKP1判断是否有升级请求 */
    if (RTC_BKP1 == 0x424C5547) {
        /* 有升级请求: 输出信息, 等10s收0x0502 */
        printf("\r\n===== BootLoader =====\r\n");

        for (i = 10; i > 0; i--) {
            printf("wait %ds for prep cmd...\r\n", i);
            if (wait_frame(1000)) {
                if (parse_frame(&cmd) && cmd == C_PREP)
                    goto recv_fw;
            }
        }
        /* 10s超时未收到0x0502, 跳APP */
        printf("prep timeout, jump to app...\r\n");
        delay_1ms(100);
        jump_to_app();
        while (1);

recv_fw:
        printf("prep OK, ACK...\r\n");
        {
            uint8_t ok = 0xFF;
            send_frame(T_ACK, C_PREP, &ok, 1);
        }

        /* 收固件(256字节切片, 500ms超时) */
        printf("receiving firmware...\r\n");
        unsigned int fw_len = recv_firmware();
        printf("received %u bytes\r\n", fw_len);

        if (fw_len < 4) {
            printf("too small, reset...\r\n");
            delay_1ms(500);
            mcu_rst();
        }

        /* 校验魔术字 */
        uint32_t magic = flash_read32(STAGING_ADDR);
        printf("magic: 0x%08X\r\n", magic);

        if (magic == FW_MAGIC) {
            printf("magic OK\r\n");
            uint8_t ok = 0xFF;
            send_frame(T_ACK, 0x0502, &ok, 1);
        } else {
            printf("magic ERROR\r\n");
            send_frame(T_ERR, 0x0502, NULL, 0);
            delay_1ms(500);
            mcu_rst();
        }

        /* 等0x0503执行升级(30s超时) */
        printf("wait exec cmd...\r\n");
        if (!wait_frame(30000))
            mcu_rst();
        if (!parse_frame(&cmd) || cmd != C_EXEC)
            mcu_rst();

        printf("exec cmd, copy...\r\n");
        {
            uint8_t ok = 0xFF;
            send_frame(T_ACK, C_EXEC, &ok, 1);
        }

        /* 擦APP区, 从暂存区搬固件到APP区 */
        {
            unsigned int pages = (fw_len + 4095) / 4096;
            printf("erase %u pages...\r\n", pages);
            for (i = 0; i < pages; i++)
                flash_erase(APP_ADDR + i * 4096);

            printf("copy %u bytes...\r\n", fw_len);
            uint8_t tmp[256];
            unsigned int off = 0;
            while (off < fw_len) {
                unsigned int chunk = (fw_len - off > 256) ? 256 : (fw_len - off);
                for (unsigned int j = 0; j < chunk; j++)
                    tmp[j] = flash_read8(STAGING_ADDR + off + j);
                flash_write(APP_ADDR + off, tmp, chunk);
                off += chunk;
            }
        }

        /* 验证前64字节 */
        printf("verify: ");
        for (i = 0; i < 64 && i < fw_len; i++)
            printf("%02X ", flash_read8(APP_ADDR + i));
        printf("\r\n");

        /* 清除升级标志, 下次正常启动 */
        rcu_periph_clock_enable(RCU_PMU);
        pmu_backup_write_enable();
        RTC_BKP1 = 0;

        printf("upgrade done, reboot...\r\n");
        delay_1ms(500);
        mcu_rst();
        while (1);
    }

    /* 无升级请求: 5s静默后跳APP */
    delay_1ms(5000);
    jump_to_app();
    while (1);
}
