#include "Func.h"
#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "U_USART1.h"
#include "W_W25QXX.h"
#include "TFT_DMA.h"

pic_infor PIC_INFO[4];
uint8_t pic_index = 1;
extern uint8_t ram_hub[1024*2];
extern uint8_t usart1_buff[64];
extern int8_t usart1_isbuff;
extern uint16_t usart1_count;
void Init_Func(void)
{
	U_Printf("Func初始化完成 \r\n");
}
void Task_Func(void* pvParameters)
{
	while(1)
	{
		ShowPic_WithFrame(pic_index);
		vTaskDelay(90);
	}
}
uint16_t param_a[5];
void Read_Pic(void)
{
	//WritePic-xxx-xxx-xxx-xxx-xxx	//传输次数-长-宽-第几张图片-第几帧
	for(int i=0;i<28;i++)
	{
		usart1_buff[i]-='0';
	}
	param_a[0] = usart1_buff[9]*100;	//次数
	param_a[0] += usart1_buff[10]*10;
	param_a[0] += usart1_buff[11];
	param_a[1] = usart1_buff[13]*100;	//宽
	param_a[1] += usart1_buff[14]*10;
	param_a[1] += usart1_buff[15];
	param_a[2] = usart1_buff[17]*100;	//高
	param_a[2] += usart1_buff[18]*10;
	param_a[2] += usart1_buff[19];
	param_a[3] = usart1_buff[21]*100;	//第几张
	param_a[3] += usart1_buff[22]*10;
	param_a[3] += usart1_buff[23];
	param_a[4] = usart1_buff[25]*100;	//第几帧
	param_a[4] += usart1_buff[26]*10;
	param_a[4] += usart1_buff[27];
//	U_Printf("次数:%d \t长:%d 宽:%d\t第%d张第%d帧 \r\n",param_a[0],param_a[1],param_a[2],param_a[3],param_a[4]);
	param_a[4] -= 1;
	uint8_t index = param_a[3];
	PIC_INFO[index].wq_times = param_a[0];
	PIC_INFO[index].width = param_a[1];
	PIC_INFO[index].height = param_a[2];
	PIC_INFO[index].pixel_count = param_a[1]*param_a[2];
	PIC_INFO[index].frame = param_a[4];
	
	uint8_t pic_index = index;
	pic_index *= 2;
	pic_index -= 1;
	uint16_t addr = (pic_index<<8)+(param_a[4]*11);
	
	
	U_Printf("ReadyToReadPic \r\n");
	vTaskDelay(100);
	U_InitDMA();	//把USART初始化为115200
	vTaskDelay(100);
	for(int i=0;i<param_a[0];i++)
	{
		WQ_Erease(addr+i);
	}
	for(int i=0;i<param_a[0];i++)
	{
		DMA_Cmd(DMA1_Channel5,DISABLE);
		DMA_SetCurrDataCounter(DMA1_Channel5,2048);
		DMA_Cmd(DMA1_Channel5,ENABLE);
		U_Printf("NEXT \r\n");	
		while(usart1_isbuff==0);
		usart1_isbuff = 0;
		WQ_RamWrite(addr+i/2,i%2);
	}
	U_Printf("FINISH \r\n");
	vTaskDelay(100);
	U_DeInitDMA();
	
//	ShowPic_WithFrame(index);
	Show_Pic(index,param_a[4]);
	WritePicInfo();
}
void Show_Pic(uint8_t pic_index,uint8_t frame_index)
{
	uint16_t temp_addr = pic_index*2;
	temp_addr -= 1;
	uint16_t addr = (temp_addr<<8)+(frame_index*11);
	uint32_t pixel_counts = PIC_INFO[pic_index].pixel_count;
	uint16_t x = (162-PIC_INFO[pic_index].width)/2;
	uint16_t y = (130-PIC_INFO[pic_index].height)/2;
	TFTD_SetRect(x,y,PIC_INFO[pic_index].width,PIC_INFO[pic_index].height);
	TFTD_Start();
	for(int i=0;i<PIC_INFO[pic_index].wq_times;i++)
	{
		//读取W25Q64
		WQ_RamRead(addr+i/2,i%2);
		//显示
		DMA_Cmd(DMA1_Channel5,DISABLE);
		if(pixel_counts<1024)
		{
			DMA_SetCurrDataCounter(DMA1_Channel5,pixel_counts);
		}
		else
		{
			DMA_SetCurrDataCounter(DMA1_Channel5,1024);
		}
		DMA_Cmd(DMA1_Channel5,ENABLE);
		DMA_ClearFlag(DMA1_FLAG_TC5);
		while(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=SET);
		DMA_ClearFlag(DMA1_FLAG_TC5);
		pixel_counts -= 1024;
	}
	TFTD_Stop();
}
void ReadPicInfo(void)
{
	WQ_RamRead(0,0);
	uint32_t* temp_ram = (uint32_t*)&ram_hub[0];
	for(int i=0;i<4;i++)
	{
		PIC_INFO[i].frame			=	temp_ram[i*10+0];
		PIC_INFO[i].height			=   temp_ram[i*10+1];
		PIC_INFO[i].index			=   temp_ram[i*10+2];
		PIC_INFO[i].pixel_count		=   temp_ram[i*10+3];
		PIC_INFO[i].width			=   temp_ram[i*10+4];
		PIC_INFO[i].wq_times		=   temp_ram[i*10+5];
		if(i==0)
		{
			continue;
		}
//		U_Printf("PIC_INFO[%d]:[%d*%d],frame:%d,pixel_counts:%d \r\n",i,PIC_INFO[i].width,PIC_INFO[i].height,PIC_INFO[i].frame+1,PIC_INFO[i].pixel_count);
	}
}
void WritePicInfo(void)
{
	WQ_Erease(0);
	uint32_t* temp_ram = (uint32_t*)&ram_hub[0];
	for(int i=0;i<4;i++)
	{
		temp_ram[i*10+0] = PIC_INFO[i].frame;
		temp_ram[i*10+1] = PIC_INFO[i].height;
		temp_ram[i*10+2] = PIC_INFO[i].index;
		temp_ram[i*10+3] = PIC_INFO[i].pixel_count;
		temp_ram[i*10+4] = PIC_INFO[i].width;
		temp_ram[i*10+5] = PIC_INFO[i].wq_times;
	}
	WQ_RamWrite(0,0);
}
void ShowPic_WithFrame(uint8_t index)
{
	uint16_t frame = PIC_INFO[index].frame;
	for(int i=0;i<=frame;i++)
	{
		Show_Pic(index,i);
	}
}



















