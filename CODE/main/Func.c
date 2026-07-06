#include "Func.h"
#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "U_USART1.h"

void Init_Func(void)
{
	U_Printf("Func初始化完成 \r\n");
}
void Task_Func(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(100);
	}
}







