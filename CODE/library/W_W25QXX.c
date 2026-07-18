#include "W_W25QXX.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"
#include "TFT_DMA.h"
	//不知道怎么写，总之直接用ram_hub了
#include "Shadow.h"

extern uint8_t ram_hub[];

/*	画了一个关于F1驱动TFT的板子...
 *	如果想显示图片的话，
 *	需要大点的Flash作为存储介质
 *	F103C8装不了SD卡
 *	所以选择了W25Q64作为存储
 *		——2025/5/28-21:32
 */
/*	关于W25Q64的机制
 *	用6位十六进制寻址
 *	0xFFFFFF
 *		前面三位0xFFF---是最小擦除单元
 *		中间第四位0x---F--是最小页写单元 
 *			    ->即每遇到0x----FF就要换页再继续页写
 *		后面两位0x----FF就是256个字节了
 *	这个驱动库以前三位(即最小擦除单元)作为寻址
 *		——2025/5/28-22:32
 */
/*	其实现在好累，什么也不想干...
 *	但想到如果什么都不干的话...
 *	就不要浪费时间了 快去学习...
 *	突然就有动力写下去了...
 *		——2025/5/28-21:30
 */
/*	参考之前库的时候发现是2025年写的...
 *	当时板子画的都挺随便的....
 *	主要时间还在写代码上....
 *	25年.....好像什么都挺好的....
 *	但现在27年....进展...虽然有....
 *	也挺多的....
 *	但就是....受了很多苦....
 *	我好难过....
 *		——2027/7/17-15:05.秦
 */

//CS# <- PB1
#define PIN_WQ_CS_L() GPIO_WriteBit(GPIOB,GPIO_Pin_1,Bit_RESET) 
#define PIN_WQ_CS_H() GPIO_WriteBit(GPIOB,GPIO_Pin_1,Bit_SET) 
//CLK <- PA5
#define PIN_WQ_CLK_L() GPIO_WriteBit(GPIOA,GPIO_Pin_5,Bit_RESET) 
#define PIN_WQ_CLK_H() GPIO_WriteBit(GPIOA,GPIO_Pin_5,Bit_SET)
//DI  <- PA7
#define PIN_WQ_DI_L() GPIO_WriteBit(GPIOA,GPIO_Pin_7,Bit_RESET) 
#define PIN_WQ_DI_H() GPIO_WriteBit(GPIOA,GPIO_Pin_7,Bit_SET)
//DO  <- PA6				//为了效率写成这样是真有毛病吧....
#define PIN_WQ_DO()		(((GPIOA->IDR&GPIO_Pin_6)==0)?0:1)

/**@brief  初始化
  *@param  void
  *@retval void
  */
void Init_WQ(void)
{	
	//引脚初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct;
		//CS引脚
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	PIN_WQ_CS_H();
		//SPI1引脚-时钟/MOSI
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
		//SPI1引脚-MISO
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	//SPI1初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);
	SPI_InitTypeDef SPI_InitStruct;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;//上升沿读取，Mode0
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CRCPolynomial = 7;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1,&SPI_InitStruct);
	SPI_Cmd(SPI1,ENABLE);
	//DMA初始化（SPI1_TX->DMA1_CH3 , SPI1_RX->DMA1_CH2）
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	DMA_InitTypeDef DMA_InitStruct;
		//SPI1_TX->DMA1_CH3
	SPI_I2S_DMACmd(SPI1,SPI_I2S_DMAReq_Tx,ENABLE);
	DMA_InitStruct.DMA_BufferSize = 2048;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ram_hub[0];
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel3,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel3,DISABLE);
		//SPI1_RX->DMA1_CH2
	SPI_I2S_DMACmd(SPI1,SPI_I2S_DMAReq_Rx,ENABLE);
	DMA_InitStruct.DMA_BufferSize = 2048;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ram_hub[0];
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel2,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel2,DISABLE);
	
	//读序列号
	WQ_Start();
	WQ_Swap(0x9F);
	U_Printf("读取W25Q64序列号[正确值EF4017]:");
	U_Printf("%h",WQ_Swap(0xFF));
	U_Printf("%h",WQ_Swap(0xFF));
	U_Printf("%h \r\n",WQ_Swap(0xFF));
	WQ_Stop();
	
	U_Printf("W25Q64初始化完成 \r\n");
	
//	//测试RAM写入
//	uint16_t* temp_ram = (uint16_t*)&ram_hub[0];
//	for(int i=0;i<1024;i++)
//	{
//		temp_ram[i] = i;
//	}
//		//擦除
//	WQ_Erease(0x002000);
//		//写入
//	WQ_RamWrite(0x002000,0);
//	for(int i=0;i<1024;i++)
//	{
//		temp_ram[i] = 0x0;
//	}
//		//读取
//	WQ_RamRead(0x002000,0);
//	for(int i=0;i<1024;i++)
//	{
//		U_Printf("%d ",temp_ram[i]);
//	}
//	U_Printf("\r\n 完成. \r\n");
}
/**@brief  来自hub的项目，把ram_hub[2048]写入地址
  *@param  addr_xxx000 地址(最小可擦除地址)
  *@param  front	   写入地址，2048=2^11，最小可擦除是2^12，所以分成上下两部分
  *@retval void
  */
void WQ_RamWrite(uint32_t addr,int8_t front)
{
	addr<<=12;
	if(front!=0)
	{
		addr += 0x800;
	}
	for(int i=0;i<0x8;i++)
	{
		//修改ram地址
		DMA_InitTypeDef DMA_InitStruct;
		DMA_InitStruct.DMA_BufferSize = 256;
		DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
		DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ram_hub[256*i];
		DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
		DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
		DMA_Init(DMA1_Channel3,&DMA_InitStruct);
		//修改写入地址
		WQ_WriteStart(addr);
		//开DMA写入
		DMA_SetCurrDataCounter(DMA1_Channel3,256);
		DMA_Cmd(DMA1_Channel3,ENABLE);
		DMA_ClearFlag(DMA1_FLAG_TC3);
		while(DMA_GetFlagStatus(DMA1_FLAG_TC3)!=SET);
		DMA_Cmd(DMA1_Channel3,DISABLE);
		//写入完成，结束通信
		WQ_WriteStop();
		WQ_WaitProcess();
		addr += 256;
	}
}
/**@brief  来自hub的项目，用ram_hub[2048]读出
  *@param  addr_xxx	 	地址，自动补充后面三个0
  *@param  front       	读前半部分
  */
void WQ_RamRead(uint32_t addr,int8_t front)
{
	addr<<=12;
	if(front!=0)
	{
		addr += 0x800;
	}
		//DMA读取
	WQ_ReadStart(addr);
		//读
	DMA_Cmd(DMA1_Channel2,DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel2,2048);
	DMA_Cmd(DMA1_Channel2,ENABLE);	
	DMA_ClearFlag(DMA1_FLAG_TC2);	
		//写
	DMA_Cmd(DMA1_Channel3,DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel3,2048);
	DMA_Cmd(DMA1_Channel3,ENABLE);
	DMA_ClearFlag(DMA1_FLAG_TC3);
	while(DMA_GetFlagStatus(DMA1_FLAG_TC3)!=SET);
	while(DMA_GetFlagStatus(DMA1_FLAG_TC2)!=SET);
	WQ_ReadStop();
	DMA_Cmd(DMA1_Channel3,DISABLE);
	DMA_Cmd(DMA1_Channel2,DISABLE);
}
/**@brief  测试内容
  */
void WQ_Test(void)
{
	//试着写入与读出
	WQ_WriteEnable();
	WQ_Erease(0x1234);
	WQ_WriteEnable();
	uint8_t test_words[10];
	for(int i=0;i<10;i++)
	{
		test_words[i] = i+1;
	}
	WQ_Write(0x1234,test_words,10);
	WQ_WaitProcess();
	for(int i=0;i<10;i++)
	{
		test_words[i] = 0;
	}
	WQ_Read(0x1234,test_words,10);
	for(int i=0;i<10;i++)
	{
		U_Printf("[%d]:%d \r\n",i,test_words[i]);
	}
	
}
/**@brief  SPI开始通信
  */
void WQ_Start(void)
{
	PIN_WQ_CS_L();
}
/**@brief  SPI结束通信
  */
void WQ_Stop(void)
{
	PIN_WQ_CS_H();
}
void WQ_WriteStart(uint32_t addr_xxxx00)
{
	WQ_WriteEnable();
	WQ_Start();
	WQ_Swap(0x02);
		//写地址
	WQ_Swap((addr_xxxx00>>16));
	WQ_Swap(((addr_xxxx00>>8)&0xFF));
	WQ_Swap((addr_xxxx00&0xFF));
}
void WQ_WriteStop()
{
	WQ_Stop();
	WQ_WaitProcess();
}
void WQ_ReadStart(uint32_t addr)
{
	WQ_Start();
	WQ_Swap(0x0B);
		//写地址
	WQ_Swap((addr>>16));
	WQ_Swap(((addr>>8)&0xFF));
	WQ_Swap((addr&0xFF));
	WQ_Swap(0xFF);
}
void WQ_ReadStop(void)
{
	WQ_Stop();
}


/**@brief  交换
  */
uint8_t WQ_Swap(uint8_t data)
{
	SPI_I2S_SendData(SPI1,data);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_TXE)!=SET);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY)==SET);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_RXNE)!=SET);
	while(SPI_I2S_GetFlagStatus(SPI1,SPI_I2S_FLAG_BSY)==SET);
	return SPI_I2S_ReceiveData(SPI1);
}
/**@brief  等待操作完成
  */
void WQ_WaitProcess(void)
{
	WQ_Start();
	WQ_Swap(0x05);
	uint8_t aaa = WQ_Swap(0x05);
	while((aaa&0x01)==1)
	{
		U_Printf("wait wq write enable\r\n");
		aaa = WQ_Swap(0x05);
	}
	WQ_Stop();
}
/**@brief  寄存器写使能
  */
void WQ_WriteEnable(void)
{
	WQ_Start();
	WQ_Swap(0x06);
	WQ_Stop();
	WQ_WaitProcess();
}
/**@brief  写入
  */
void WQ_Write(uint32_t addr_xxxx00,uint8_t* bytes,uint16_t length)
{
	WQ_Start();
	WQ_Swap(0x02);
		//写地址
	WQ_Swap((addr_xxxx00>>16));
	WQ_Swap(((addr_xxxx00>>8)&0xFF));
	WQ_Swap((addr_xxxx00&0xFF));
		//写数据
	for(int i=0;i<length;i++)
	{
		WQ_Swap(bytes[i]);
	}
	WQ_Stop();
}
/**@brief  读取
  */
void WQ_Read(uint32_t addr,uint8_t* bytes,uint16_t length)
{
	WQ_Start();
	WQ_Swap(0x0B);
		//写地址
	WQ_Swap((addr>>16));
	WQ_Swap(((addr>>8)&0xFF));
	WQ_Swap((addr&0xFF));
	WQ_Swap(0xFF);
		//读数据
	for(int i=0;i<length;i++)
	{
		bytes[i] = WQ_Swap(0xFF);
	}
	WQ_Stop();
}
/**@brief  擦除
  */
void WQ_Erease(uint32_t addr_xxx)
{	
	addr_xxx<<=12;
	WQ_WriteEnable();
	WQ_Start();
	WQ_Swap(0x20);
		//写地址
	WQ_Swap((addr_xxx>>16));
	WQ_Swap(((addr_xxx>>8)&0xFF));
	WQ_Swap((addr_xxx&0xFF));
	WQ_Stop();
	WQ_WaitProcess();
}









