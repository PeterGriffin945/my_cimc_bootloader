/************************************************************
 * 版权：2025CIMC Copyright。
 * 文件：RTC.c
 * 作者: Qiao Qin @ GigaDevice
 * 平台: 2025CIMC IHD-V04
 * 版本: Qiao Qin     2025/4/20     V0.01    original
 ************************************************************/

#include "RTC.h"
#include "LED.h"

#define RTC_CLOCK_SOURCE_LXTAL // 配置RTC时钟源
// #define RTC_CLOCK_SOURCE_IRC32K
#define BKP_VALUE 0x32F0

rtc_parameter_struct rtc_initpara;
rtc_alarm_struct rtc_alarm;
__IO uint32_t prescaler_a = 0, prescaler_s = 0;
uint32_t RTCSRC_FLAG = 0;

/*!
    \brief      main function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void RTC_Init(void)
{
    printf("\n\r  ****************** RTC calendar demo ******************\n\r");

    /* enable PMU clock */
    rcu_periph_clock_enable(RCU_PMU);
    /* enable the access of the RTC registers */
    pmu_backup_write_enable();

    rtc_pre_config();
    /* get RTC clock entry selection */
    RTCSRC_FLAG = !GET_BITS(RCU_BDCTL, 8, 9);

    /* check if RTC has aready been configured */
    if ((BKP_VALUE != RTC_BKP0) || (0x00 == RTCSRC_FLAG))
    {
        /* backup data register value is not correct or not yet programmed
        or RTC clock source is not configured (when the first time the program
        is executed or data in RCU_BDCTL is lost due to Vbat feeding) */
				printf("entering rtc_setup...\r\n"); 
        rtc_setup();
    }
    else
    {
        /* detect the reset source */
        if (RESET != rcu_flag_get(RCU_FLAG_PORRST))
        {
            printf("power on reset occurred....\n\r");
        }
        else if (RESET != rcu_flag_get(RCU_FLAG_EPRST))
        {
            printf("external reset occurred....\n\r");
        }
        printf("no need to configure RTC....\n\r");

        rtc_show_time();
    }
    rcu_all_reset_flag_clear();
}

/*!
    \brief      RTC configuration function
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_pre_config(void)
{
#if defined(RTC_CLOCK_SOURCE_IRC32K)
    rcu_osci_on(RCU_IRC32K);
    rcu_osci_stab_wait(RCU_IRC32K);
    rcu_rtc_clock_config(RCU_RTCSRC_IRC32K);

    prescaler_s = 0x13F;
    prescaler_a = 0x63;
#elif defined(RTC_CLOCK_SOURCE_LXTAL)
    rcu_osci_on(RCU_LXTAL);
    rcu_osci_stab_wait(RCU_LXTAL);
    rcu_rtc_clock_config(RCU_RTCSRC_LXTAL);

    prescaler_s = 0xFF;
    prescaler_a = 0x7F;
#else
#error RTC clock source should be defined.
#endif /* RTC_CLOCK_SOURCE_IRC32K */

    rcu_periph_clock_enable(RCU_RTC);
    rtc_register_sync_wait();
}

/*!
    \brief      use hyperterminal to setup RTC time and alarm
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_setup(void)
{
    /* setup RTC time value */
    uint32_t tmp_year = 0xFF, tmp_month = 0xFF, tmp_day = 0xFF;
    uint32_t tmp_hh = 0xFF, tmp_mm = 0xFF, tmp_ss = 0xFF;

    rtc_initpara.factor_asyn = prescaler_a;
    rtc_initpara.factor_syn = prescaler_s;
    rtc_initpara.year = 0x16;
    rtc_initpara.day_of_week = RTC_SATURDAY;
    rtc_initpara.month = RTC_APR;
    rtc_initpara.date = 0x30;
    rtc_initpara.display_format = RTC_24HOUR;
    rtc_initpara.am_pm = RTC_AM;

    /* current time input */
    printf("=======Configure RTC Time========\n\r");
    printf("  please set the last two digits of current year:\n\r");
    while (tmp_year == 0xFF)
    {
        tmp_year = usart_input_threshold(99);
        rtc_initpara.year = tmp_year;
    }
    printf("  20%0.2x\n\r", tmp_year);

    printf("  please input month:\n\r");
    while (tmp_month == 0xFF)
    {
        tmp_month = usart_input_threshold(12);
        rtc_initpara.month = tmp_month;
    }
    printf("  %0.2x\n\r", tmp_month);

    printf("  please input day:\n\r");
    while (tmp_day == 0xFF)
    {
        tmp_day = usart_input_threshold(31);
        rtc_initpara.date = tmp_day;
    }
    printf("  %0.2x\n\r", tmp_day);

    printf("  please input hour:\n\r");
    while (0xFF == tmp_hh)
    {
        tmp_hh = usart_input_threshold(23);
        rtc_initpara.hour = tmp_hh;
    }
    printf("  %0.2x\n\r", tmp_hh);

    printf("  please input minute:\n\r");
    while (0xFF == tmp_mm)
    {
        tmp_mm = usart_input_threshold(59);
        rtc_initpara.minute = tmp_mm;
    }
    printf("  %0.2x\n\r", tmp_mm);

    printf("  please input second:\n\r");
    while (0xFF == tmp_ss)
    {
        tmp_ss = usart_input_threshold(59);
        rtc_initpara.second = tmp_ss;
    }
    printf("  %0.2x\n\r", tmp_ss);

    /* RTC current time configuration */
    if (ERROR == rtc_init(&rtc_initpara))
    {
        printf("\n\r** RTC time configuration failed! **\n\r");
        LED_Control(LED_1,LED_ON);
    }
    else
    {
        printf("\n\r** RTC time configuration success! **\n\r");
        rtc_show_time();
        RTC_BKP0 = BKP_VALUE;
				LED_Control(LED_1,LED_ON);
    }

    //    /* setup RTC alarm */
    //    tmp_hh = 0xFF;
    //    tmp_mm = 0xFF;
    //    tmp_ss = 0xFF;

    //    rtc_alarm_disable(RTC_ALARM0);
    //    printf("=======Input Alarm Value=======\n\r");
    //    rtc_alarm.alarm_mask = RTC_ALARM_DATE_MASK|RTC_ALARM_HOUR_MASK|RTC_ALARM_MINUTE_MASK;
    //    rtc_alarm.weekday_or_date = RTC_ALARM_DATE_SELECTED;
    //    rtc_alarm.alarm_day = 0x31;
    //    rtc_alarm.am_pm = RTC_AM;

    //    /* RTC alarm input */
    //    printf("  please input Alarm Hour:\n\r");
    //    while (0xFF == tmp_hh){
    //        tmp_hh = usart_input_threshold(23);
    //        rtc_alarm.alarm_hour = tmp_hh;
    //    }
    //    printf("  %0.2x\n\r", tmp_hh);

    //    printf("  Please Input Alarm Minute:\n\r");
    //    while (0xFF == tmp_mm){
    //        tmp_mm = usart_input_threshold(59);
    //        rtc_alarm.alarm_minute = tmp_mm;
    //    }
    //    printf("  %0.2x\n\r", tmp_mm);

    //    printf("  Please Input Alarm Second:\n\r");
    //    while (0xFF == tmp_ss){
    //        tmp_ss = usart_input_threshold(59);
    //        rtc_alarm.alarm_second = tmp_ss;
    //    }
    //    printf("  %0.2x", tmp_ss);

    //    /* RTC alarm configuration */
    //    rtc_alarm_config(RTC_ALARM0,&rtc_alarm);
    //    printf("\n\r** RTC Set Alarm Success!  **\n\r");
    //    rtc_show_alarm();

    //    rtc_interrupt_enable(RTC_INT_ALARM0);
    //    rtc_alarm_enable(RTC_ALARM0);
}

/*!
    \brief      display the current time
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_show_time(void)
{
    //    uint32_t time_subsecond = 0;
    //    uint8_t subsecond_ss = 0,subsecond_ts = 0,subsecond_hs = 0;

    rtc_current_time_get(&rtc_initpara);

    /* get the subsecond value of current time, and convert it into fractional format */
    //    time_subsecond = rtc_subsecond_get();
    //    subsecond_ss=(1000-(time_subsecond*1000+1000)/400)/100;
    //    subsecond_ts=(1000-(time_subsecond*1000+1000)/400)%100/10;
    //    subsecond_hs=(1000-(time_subsecond*1000+1000)/400)%10;

    printf("\r\nCurrent time: 20%0.2x-%0.2x-%0.2x",
           rtc_initpara.year, rtc_initpara.month, rtc_initpara.date);

    printf(" : %0.2x:%0.2x:%0.2x \r\n",
           rtc_initpara.hour, rtc_initpara.minute, rtc_initpara.second);
}

/*!
    \brief      display the alram value
    \param[in]  none
    \param[out] none
    \retval     none
*/
void rtc_show_alarm(void)
{
    rtc_alarm_get(RTC_ALARM0, &rtc_alarm);
    printf("The alarm: %0.2x:%0.2x:%0.2x \n\r", rtc_alarm.alarm_hour, rtc_alarm.alarm_minute,
           rtc_alarm.alarm_second);
}

/*!
    \brief      get the input character string and check if it is valid
    \param[in]  none
    \param[out] none
    \retval     input value in BCD mode
*/
uint8_t usart_input_threshold(uint32_t value)
{
    uint32_t index = 0;
    uint32_t tmp[2] = {0, 0};

    while (index < 2)
    {
        while (RESET == usart_flag_get(USART0, USART_FLAG_RBNE))
            ;
        tmp[index++] = usart_data_receive(USART0);
        if ((tmp[index - 1] < 0x30) || (tmp[index - 1] > 0x39))
        {
            printf("\n\r please input a valid number between 0 and 9 \n\r");
            index--;
        }
    }

    //字符串转数字 '0'的ASCII码是0x30，所以要减去0x30才能得到数字
    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) * 10);
    if (index > value)
    {
        printf("\n\r please input a valid number between 0 and %d \n\r", value);
        return 0xFF;
    }

    //十进制转BCD码，每个数字占4位，所以要左移4位
    index = (tmp[1] - 0x30) + ((tmp[0] - 0x30) << 4);
    return index;
}


//新增自定义函数，用来返回一个包含日期时间信息的数组
//param[out] date_str  日期字符串，格式 "20xx-xx-xx"，调用者需保证buf >= 11字节
//param[out] time_str  时间字符串，格式 "xx:xx:xx"  ，调用者需保证buf >= 9字节

void RTC_GetTimeString(char *date_str, char *time_str)
{
    rtc_current_time_get(&rtc_initpara);

    /* 日期：20xx-xx-xx */
    sprintf(date_str, "20%02x-%02x-%02x",
            rtc_initpara.year,
            rtc_initpara.month,
            rtc_initpara.date);

    /* 时间：xx:xx:xx */
    sprintf(time_str, "%02x:%02x:%02x",
            rtc_initpara.hour,
            rtc_initpara.minute,
            rtc_initpara.second);
}
