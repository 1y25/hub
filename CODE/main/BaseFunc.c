#include "BaseFunc.h"
#include "Shadow.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"

#define BF_LED_RCC_GPIOX	RCC_APB2Periph_GPIOA
#define BF_LED_GPIOX		GPIOA
#define BF_LED_Pin			GPIO_Pin_8

/**@brief  启动时相关的初始化函数
  *@param  void
  *@retval void
  */
void Start_Func(void)
{
	//串口初始化应该放在最前
	Start_CommandInit();
	Start_LEDInit();
	Start_WDGInit(3000);

	taskENTER_CRITICAL();
	TaskHandle_t START_LEDTASK_Handler;
	xTaskCreate(Start_LEDTask,"Start_LED",32,NULL,8,&START_LEDTASK_Handler);
	TaskHandle_t START_COMMANDTASK_Handler;
	xTaskCreate(Start_CommandTask,"Start_Command",128,NULL,5,&START_COMMANDTASK_Handler);
	taskEXIT_CRITICAL();
}
/**@brief  用于指示运行的LED
  *@param  void
  *@retval void
  */
void Start_LEDInit(void)
{
	RCC_APB2PeriphClockCmd(BF_LED_RCC_GPIOX,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = BF_LED_Pin;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(BF_LED_GPIOX,&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	U_Printf("LED初始化完成 \r\n");
}
/**@brief  看门狗初始化
  *@param  wdg_ms 看门狗复位毫秒数
  *@retval void
  */
void Start_WDGInit(uint16_t wdg_ms)
{
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST)==SET)
	{
		U_Printf("警告！触发看门狗复位. \r\n");
		RCC_ClearFlag();
	}
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_32);
	IWDG_SetReload(wdg_ms);
	IWDG_Enable();
	U_Printf("看门狗已启动,重装值:%dms \r\n",wdg_ms);
	IWDG_ReloadCounter();
}
/**@brief  指示LED线程
  *@param  void
  *@retval void
  */
void Start_LEDTask(void* pvParameters)
{
	while(1)
	{
		IWDG_ReloadCounter();
		GPIO_WriteBit(BF_LED_GPIOX,BF_LED_Pin,Bit_RESET);
		vTaskDelay(400);
		GPIO_WriteBit(BF_LED_GPIOX,BF_LED_Pin,Bit_SET);
		vTaskDelay(600);
	}
}
/**@brief  指令监听初始化
  *@param  void
  *@retval void
  */
void Start_CommandInit(void)
{
	U_USART1_Init();
	U_Printf("指令监听开启 \r\n");
}
/**@brief  指令监听线程
  *@param  void
  *@retval void
  */
extern int8_t usart1_isbuff;
void Start_CommandTask(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(20);
		if(usart1_isbuff==0)
		{
			continue;
		}
		if(Start_CommandFunc()!=0)
		{
			usart1_isbuff=0;
			continue;
		}
		if(Command("COMMAND")||Command("HELP"))
		{
			U_Printf("这里是F103程序默认COMMAND \r\n");
		}
		else if(Command("STACK"))
		{
			uint16_t numberoftask = uxTaskGetNumberOfTasks();
			//申请内存时建议进入临界区
			taskENTER_CRITICAL();
			TaskStatus_t* task_status = pvPortMalloc(sizeof(TaskStatus_t)*numberoftask);
			taskEXIT_CRITICAL();
			uxTaskGetSystemState(task_status,numberoftask,NULL);
			U_Printf("已获取当前线程剩余栈:\r\n");
			for(int i=0;i<numberoftask;i++)
			{
				U_Printf("%d\t%s\r\n",task_status[i].usStackHighWaterMark,task_status[i].pcTaskName);
			}
			//释放内存
			vPortFree(task_status);
		}
		else if(Command("RESET"))
		{
			U_Printf("即将重置\r\n");
			vTaskDelay(1000);
			NVIC_SystemReset();
		}
		else
		{
			U_Printf("指令错误 \r\n建议使用'COMMAND'或者'HELP' \r\n");
		}
		usart1_isbuff=0;
	}
}


