/*  启动函数库  */
#include "Shadow.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"

int main(void)
{
	//创建开始线程
	TaskHandle_t START_MAINTASK_Handler;
	xTaskCreate(Start_MainTask,"Start_Main",128,NULL,8,&START_MAINTASK_Handler);
	//开启任务调度器
	vTaskStartScheduler();
	while(1)
	{
		//其实这个while疑似没用
	}
}







