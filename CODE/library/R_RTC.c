#include "R_RTC.h"
#include "stdint.h"
#include "time.h"
/*  ST库  */
#include "stm32f10x.h"
/*  外设库  */
#include "U_USART1.h"

uint16_t 	year 	=2026;
uint8_t 	month 	=6;
uint8_t 	day 	=26;
uint8_t 	hour	=15;
uint8_t 	minute	=57;
uint8_t 	second	=12;
uint32_t	rtc_counter = 1111;

/**@brief  把全局变量(十分秒)转换为rtc_counter
  */
uint16_t R_SetCounter(void)
{
	struct tm rtc_time;
	rtc_time.tm_year 	= year -1900;
	rtc_time.tm_mon 	= month;
	rtc_time.tm_mday 	= day;
	rtc_time.tm_hour 	= hour;
	rtc_time.tm_min 	= minute;
	rtc_time.tm_sec 	= second;
	rtc_counter = (uint32_t)mktime(&rtc_time);
	return rtc_counter;
}
/**@breif  把rtc_counter转为时分秒
  */
struct tm* R_SetTime(void)
{
	struct tm* rtc_time = localtime((const time_t*)&rtc_counter);
	year = 	rtc_time->tm_year+1900 	;
	month = rtc_time->tm_mon 	;
	day = 	rtc_time->tm_mday 	    ;
	hour = 	rtc_time->tm_hour 	;
	minute =rtc_time->tm_min 	;
	second =rtc_time->tm_sec 	;
	
	return rtc_time;
}
/**@breif  打印rtc_time
  */
void R_PrintTime(void)
{
	rtc_counter = RTC_GetCounter();
	struct tm* rtc_time = localtime((const time_t*)&rtc_counter);
	U_Printf("RTC:%d/%d/%d",rtc_time->tm_year+1900,rtc_time->tm_mon,rtc_time->tm_mday);
	U_Printf("-%d/%d/%d \r\n",rtc_time->tm_hour,rtc_time->tm_min,rtc_time->tm_sec);
}

void Init_RTC(void)
{	
	//时钟初始化
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR|RCC_APB1Periph_BKP,ENABLE);
	//备用电源使能
	PWR_BackupAccessCmd(ENABLE);
//	BKP_DeInit();
	//使用外部晶振
	RCC_LSEConfig(RCC_LSE_ON);
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY)==RESET);	//RESET->晶振不稳定
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
	RCC_RTCCLKCmd(ENABLE);
	//配置RTC
	RTC_EnterConfigMode();
	RTC_SetPrescaler(32768-1);
	RTC_WaitForLastTask();
	RTC_ExitConfigMode();
	//设置初始时间
	if(BKP_ReadBackupRegister(BKP_DR1)!=0x77)
	{
		BKP_WriteBackupRegister(BKP_DR1,0x77);
		U_Printf("RTC时钟未被初始化，现在初始化为 \r\n");
		R_SetCounter();
		RTC_SetCounter(rtc_counter);
		RTC_WaitForLastTask();
		R_PrintTime();
		U_Printf("->%h \r\n",BKP_ReadBackupRegister(BKP_DR1));
	}
	else 
	{
		U_Printf("RTC时钟已经初始化完成: \r\n");
		R_SetTime();
		R_PrintTime();
	}
}
void Task_RTC(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(1000);
		rtc_counter = RTC_GetCounter();
		R_SetTime();
		R_PrintTime();
	}
}
