#include "A_ADC.h"
/*  ST库  */
#include "stm32f10x.h"
/*  外设库  */
#include "U_USART1.h"

/*	www不太想写软件...
 *	总感觉开发环境不舒服...
 *	最近要做的事情太多了..
 *	代码都写不顺心
 *		————2026/5/20-13:08.秦羽
 */

uint32_t adc_value[5];

/**@brief  ADC初始化
  */
void Init_ADC(void)
{
	//外设时钟初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	//引脚初始化(PA1->ADC12_IN1)
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	//ADC时钟分频
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);
	//设置ADC通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,1,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_3,2,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_4,3,ADC_SampleTime_55Cycles5);
	//外设初始化
	ADC_InitTypeDef ADC_InitStruct;
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStruct.ADC_NbrOfChannel = 3;
	ADC_InitStruct.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1,&ADC_InitStruct);
	//DMA初始化(DMA1_Channel1)
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = 3;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)adc_value;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel1,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel1,ENABLE);
	//开启ADC的DMA模式
	ADC_DMACmd(ADC1,ENABLE);
	//启用ADC
	ADC_Cmd(ADC1,ENABLE);
	//ADC校准
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	
	U_Printf("ADC初始化->%d \r\n",adc_value[0]);
}

/**@brief  线程：每半秒采集一次
  */
void Task_ADC(void* pvParameters)
{
	uint32_t value_display[4];
	uint8_t count = 0;		//采集100次数据求平均值
	for(int i=0;i<(sizeof(value_display)/sizeof(uint32_t));i++)
	{
		value_display[i] = 0;
	}
	while(1)
	{
		vTaskDelay(10);
		if(count>=100)
		{
			for(int i=0;i<3;i++)
			{
				value_display[i] /= count;
				value_display[i] *= 3300;
				value_display[i] /= 4095;
				value_display[i] /= 5;
			}
			count = 0;
			U_Printf("ADC: %dmA\t%dmA\t%dmA\t%dmA \r\n",value_display[0],value_display[1],value_display[2],value_display[3]);
			for(int i=0;i<3;i++)
			{
				value_display[i] = 0;
			}
		}
		for(int i=0;i<3;i++)
		{
			value_display[i] += adc_value[i];
		}
		count++;
		
	}
}












