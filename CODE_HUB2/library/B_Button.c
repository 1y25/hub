#include "B_Button.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"

#define BUTTON_TEXT	"PB14/PB12/PB11/PB10/PA5"

/**@brief  按键初始化
  */
void Init_Button(void)
{	
	//时钟初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB,ENABLE);
	//引脚初始化
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
		//PB
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_14;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
		//PA
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	
	U_Printf("Button初始化完成-%s \r\n",BUTTON_TEXT);
}
static int8_t Button_Test(GPIO_TypeDef* GPIOX,uint16_t GPIO_Pin)
{
	if(GPIO_ReadInputDataBit(GPIOX,GPIO_Pin)==Bit_RESET)
	{
		vTaskDelay(10);
		while(GPIO_ReadInputDataBit(GPIOX,GPIO_Pin)==Bit_RESET);
		vTaskDelay(10);
		return 1;
	}
	return 0;
}
/**@breif  按键线程
  */
void Task_Button(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(50);
		if(Button_Test(GPIOB,GPIO_Pin_10))
		{
			U_Printf("PB10 -> \r\n");
		}
		else if(Button_Test(GPIOB,GPIO_Pin_11))
		{
			U_Printf("PB11 -> \r\n");
		}
		else if(Button_Test(GPIOA,GPIO_Pin_5))
		{
			U_Printf("PA5 -> \r\n");
		}
		else if(Button_Test(GPIOB,GPIO_Pin_12))
		{
			U_Printf("PB12 -> \r\n");
		}
		else if(Button_Test(GPIOB,GPIO_Pin_14))
		{
			U_Printf("PB14 -> \r\n");
		}
		
	}
}







