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

#define ADC_VREF	1205	//校对值 1200+-30
uint16_t adc_value[5] = {0,0,0,0,0};
uint8_t adc_count = 0;//采样次数
uint32_t temp_value[5] = {0,0,0,0,0};

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
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	//ADC时钟分频
	RCC_ADCCLKConfig(RCC_PCLK2_Div8);
	//启用参考电压通道
	ADC_TempSensorVrefintCmd(ENABLE);
	//设置ADC通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_Vrefint,1,ADC_SampleTime_239Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,2,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_3,3,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_4,4,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,5,ADC_SampleTime_55Cycles5);
	//外设初始化
	ADC_InitTypeDef ADC_InitStruct;
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStruct.ADC_NbrOfChannel = 5;
	ADC_InitStruct.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1,&ADC_InitStruct);
	//DMA初始化(DMA1_Channel1)
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = 5;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)adc_value;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Low;
	DMA_Init(DMA1_Channel1,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel1,ENABLE);
	//开启ADC的DMA模式
	ADC_DMACmd(ADC1,ENABLE);
	//启用ADC
	ADC_Cmd(ADC1,ENABLE);
	//ADC校准
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1)!=RESET);
	
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	
	U_Printf("ADC初始化->");
	for(int i=0;i<5;i++)
	{
		U_Printf("[%d]:%d ",i,adc_value[i]);
	}
	U_Printf("\r\n");
}

/**@brief  线程：每半秒采集一次
  */
void Task_ADC(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(10);//25次->每1/4s输出一次
		if(adc_count>=25)
		{
			//数据处理
				//获得真实电压
			for(int i=0;i<5;i++)
			{
				temp_value[i]  /= adc_count;
			}
			for(int i=1;i<5;i++)
			{
				temp_value[i] *= ADC_VREF;
				temp_value[i] /= temp_value[0];
			}
				//电流处理(INA180A2[50倍率]采样0.1R)->value*10/50->value/5
			for(int i=1;i<4;i++)
			{
				temp_value[i] /= 5;
			}
				//电压处理 5V-100K-采样点-30K-GND  ->  value/30*130->value/3*13
			temp_value[4] *= 13;
			temp_value[4] /= 3;
			//数据显示
			U_Printf("ADC电流:");
			for(int i=1;i<4;i++)
			{
				U_Printf("%d ",temp_value[i]);
			}
			U_Printf(" 电压:%d \r\n",temp_value[4]);
			
			//清空数据
			adc_count = 0;
			for(int i=0;i<5;i++)
			{
				temp_value[i] = 0;
			}
		}
		
		//多次采样
		for(int i=0;i<5;i++)
		{
			temp_value[i] += adc_value[i];
		}
		adc_count++;
		
	}
}












