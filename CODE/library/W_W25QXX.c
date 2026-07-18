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
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
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
	//读序列号
	WQ_Start();
	WQ_Swap(0x9F);
	U_Printf("读取W25Q64序列号[正确值EF4017]:");
	U_Printf("%h",WQ_Swap(0xFF));
	U_Printf("%h",WQ_Swap(0xFF));
	U_Printf("%h \r\n",WQ_Swap(0xFF));
	WQ_Stop();
	//试着写入与读出
	
	U_Printf("W25Q64初始化完成 \r\n");
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
/**@brief  寄存器写使能
  */
void WQ_WriteEnable(void)
{
	WQ_Start();
	WQ_Swap(0x06);
	WQ_Swap(0xFF);
	WQ_Stop();
}








