#include "W_W25QXX.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"
#include "TFT_DMA.h"

void Init_WQ(void)
{
	U_Printf("W25Q16初始化完成 \r\n");
}
void Task_WQ(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(1000);
	}
}
void Cmd_WQ(void)
{	

}









