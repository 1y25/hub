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
extern uint8_t ram_hub[1024*2];
int8_t wq_trans = 0;
int8_t wq_usart = 0;
extern const unsigned char gImage_miku[];
extern const unsigned char IMG_120_68[];
extern int8_t usart1_buff[];
void Cmd_WQ(void)
{	
	wq_trans = 1;
	TFTD_SetRect(0,10,42,23);
	TFTD_Start();
	//USART_DMA重新初始化
	DMA_Cmd(DMA1_Channel5,DISABLE);
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = 1024*2;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)ram_hub;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_High;
	DMA_Init(DMA1_Channel5,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel5,ENABLE);
	//等待传输
	U_Printf("WQ准备完成 等待串口传输 \r\n");
	while(wq_usart==0);
	wq_usart = 0;
	//TFT_DMA重新初始化
	DMA_Cmd(DMA1_Channel3,DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel3,42*23);
	DMA_Cmd(DMA1_Channel3,ENABLE);
	while(DMA_GetFlagStatus(DMA1_FLAG_TC3)!=SET);
	DMA_ClearFlag(DMA1_FLAG_TC3);
	//串口重新初始化
	DMA_Cmd(DMA1_Channel5,DISABLE);
	DMA_InitStruct.DMA_BufferSize = 254;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)usart1_buff;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel5,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel5,ENABLE);
	
	U_Printf("WQ传输完成 \r\n");
	wq_trans = 0;
}









