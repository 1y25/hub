#include "TFT_DMA.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"

/*	当前是在写hub的屏幕驱动
 *	现在还在无人机公司做实习...
 *	硬件岗，以后可能就业都是干硬件工程师了...
 *	不是很情愿写程序了....
 *	之后把那个大的电路板做完了还是用CodeX编程吧...
 *		2026/7/4-10:14
 */
/*	SPI关键引脚：PB13/PB15 (SPI2->DMA1_CH5)
 */
/*	当前在进行>拓展坞v2<的屏幕适配
 *	PB13	->SCK
 *	PB15	->MOSI
 *	PB14	->LED+
 *	PB11	->RST
 *	PB10	->DC
 *	PB12	->CS
 */

//RST ->PB11
#define PIN_TFTD_RST_High()	GPIOB->BSRR = GPIO_Pin_11
#define PIN_TFTD_RST_Low()	GPIOB->BRR  = GPIO_Pin_11
//DC  ->PB10
#define PIN_TFTD_DC_Data()	GPIOB->BSRR = GPIO_Pin_10
#define PIN_TFTD_DC_Cmd()	GPIOB->BRR  = GPIO_Pin_10
//CS  ->PB12
#define PIN_TFTD_CS_High()	GPIOB->BSRR = GPIO_Pin_12
#define PIN_TFTD_CS_Low()	GPIOB->BRR  = GPIO_Pin_12

/**@brief  引脚初始化
  */
static void Init_TFTD_Pin(void)
{
	//时钟初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	//引脚初始化
		//SPI2复用引脚
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_13;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
		//其他引脚
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_14;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	//给初始电平
	GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_SET);
}

#include "qy_pic.h"
extern const unsigned char IMG_120_68[];
static void TFTD_SoftwareInit(void);
extern uint8_t ram_hub[];
void Init_TFTD(void)
{
	//时钟初始化
		//SPI2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,ENABLE);
		//DMA1
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	//引脚初始化
	Init_TFTD_Pin();
	//SPI2初始化
	SPI_InitTypeDef SPI_InitStruct;
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CRCPolynomial = 7;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI2,&SPI_InitStruct);
	//启用SPI
	SPI_Cmd(SPI2,ENABLE);
	//软件初始化
	vTaskDelay(200);
	TFTD_SoftwareInit();
	//初始化完成后配置成16位
	SPI_Cmd(SPI2,DISABLE);
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_16b;
	SPI_Init(SPI2,&SPI_InitStruct);
	SPI_Cmd(SPI2,ENABLE);
	//DMA初始化
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = 1024;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)&ram_hub[0];
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel5,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel5,DISABLE);
	SPI_I2S_DMACmd(SPI2,SPI_I2S_DMAReq_Tx,ENABLE);
	DMA_ClearFlag(DMA1_FLAG_TC5);
	
	//测试图像
		//蓝粉白
	uint8_t width = 133/3 +1;
	uint16_t blue  = (uint16_t)TFT_RGB888To565(0x5FCDE4);
	uint16_t white = (uint16_t)TFT_RGB888To565(0xFFFFFF); 
	uint16_t pink  = (uint16_t)TFT_RGB888To565(0xFFB6C1);
	TFTD_SetRect(0,0,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFTD_WriteData16(pink);
	}
	TFTD_SetRect(0,width,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFTD_WriteData16(white);
	}
	TFTD_SetRect(0,width*2,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFTD_WriteData16(blue);
	}
		//输出Smol Miku
	TFTD_SetRect(20,30,120,68);
			//开始通信
	TFTD_Start();
			//DMA输出处理
	for(int j=0;j<10;j++)
	{
		for(int i=0;i<12*68*2;i++)
		{
			ram_hub[i] = IMG_120_68[j*12*68*2+i];
		}
		DMA_Cmd(DMA1_Channel5,DISABLE);
		DMA_SetCurrDataCounter(DMA1_Channel5,12*68);
		DMA_Cmd(DMA1_Channel5,ENABLE);
		while(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=SET);
		DMA_ClearFlag(DMA1_FLAG_TC5);
	}
			//结束通信
	TFTD_Stop();
	
	U_Printf("TFT_withDMA初始化完成 \r\n");
}
void Task_TFTD(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(100);
	}
}
void Cmd_TFTD(void)
{

}
void TFTD_WriteCmd(uint8_t cmd)
{
	PIN_TFTD_DC_Cmd();
	PIN_TFTD_CS_Low();
	SPI_I2S_SendData(SPI2,cmd);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE)!=SET);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY)==SET);
	PIN_TFTD_CS_High();
}
void TFTD_WriteData(uint8_t data)
{
	PIN_TFTD_DC_Data();
	PIN_TFTD_CS_Low();
	SPI_I2S_SendData(SPI2,data);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE)!=SET);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY)==SET);
	PIN_TFTD_CS_High();
}
void TFTD_SetRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{
	TFTD_WriteCmd(0x2a);
	TFTD_WriteData16(y);
	TFTD_WriteData16(y+height-1);
	
	TFTD_WriteCmd(0x2b);
	TFTD_WriteData16(x);
	TFTD_WriteData16(x+width-1);
	
	TFTD_WriteCmd(0x2c);
}
void TFTD_WriteData16(uint16_t rgb565)
{
	//数据
	PIN_TFTD_DC_Data();
	//片选选中	
	PIN_TFTD_CS_Low();
	SPI_I2S_SendData(SPI2,rgb565);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_TXE)!=SET);
	while(SPI_I2S_GetFlagStatus(SPI2,SPI_I2S_FLAG_BSY)==SET);
	//片选结束
	PIN_TFTD_CS_High();
}
void TFTD_Start(void)
{
	PIN_TFTD_DC_Data();
	PIN_TFTD_CS_Low();
}
void TFTD_Stop(void)
{
	PIN_TFTD_CS_High();
}
static void TFTD_SoftwareInit(void)
{
//Reset before LCD Init.
	PIN_TFTD_RST_Low();
	vTaskDelay(100);
	PIN_TFTD_RST_High();
	vTaskDelay(50);

	//LCD Init For 1.44Inch LCD Panel with ST7735R.
	TFTD_WriteCmd(0x11);//Sleep exit 
	vTaskDelay(120);
		
	//ST7735R Frame Rate
	TFTD_WriteCmd(0xB1); 
	TFTD_WriteData(0x01); 
	TFTD_WriteData(0x2C); 
	TFTD_WriteData(0x2D); 

	TFTD_WriteCmd(0xB2); 
	TFTD_WriteData(0x01); 
	TFTD_WriteData(0x2C); 
	TFTD_WriteData(0x2D); 

	TFTD_WriteCmd(0xB3); 
	TFTD_WriteData(0x01); 
	TFTD_WriteData(0x2C); 
	TFTD_WriteData(0x2D); 
	TFTD_WriteData(0x01); 
	TFTD_WriteData(0x2C); 
	TFTD_WriteData(0x2D); 
	
	TFTD_WriteCmd(0xB4); //Column inversion 
	TFTD_WriteData(0x07); 
	
	//ST7735R Power Sequence
	TFTD_WriteCmd(0xC0); 
	TFTD_WriteData(0xA2); 
	TFTD_WriteData(0x02); 
	TFTD_WriteData(0x84); 
	TFTD_WriteCmd(0xC1); 
	TFTD_WriteData(0xC5); 

	TFTD_WriteCmd(0xC2); 
	TFTD_WriteData(0x0A); 
	TFTD_WriteData(0x00); 

	TFTD_WriteCmd(0xC3); 
	TFTD_WriteData(0x8A); 
	TFTD_WriteData(0x2A); 
	TFTD_WriteCmd(0xC4); 
	TFTD_WriteData(0x8A); 
	TFTD_WriteData(0xEE); 
	
	TFTD_WriteCmd(0xC5); //VCOM 
	TFTD_WriteData(0x0E); 
	
	//Y反转-X反转-XY调换-Y刷新方向-RGB(0)/BGR(1)-X刷新方向-0-0
	//0xC0(1100 0000)->(Y反转-X反转-XY不调换-0 0000)
	//0110 0000
	TFTD_WriteCmd(0x36); //MX, MY, RGB mode 
	TFTD_WriteData(0x80); //1000 0000
	
	//ST7735R Gamma Sequence
	TFTD_WriteCmd(0xe0); 
	TFTD_WriteData(0x0f); 
	TFTD_WriteData(0x1a); 
	TFTD_WriteData(0x0f); 
	TFTD_WriteData(0x18); 
	TFTD_WriteData(0x2f); 
	TFTD_WriteData(0x28); 
	TFTD_WriteData(0x20); 
	TFTD_WriteData(0x22); 
	TFTD_WriteData(0x1f); 
	TFTD_WriteData(0x1b); 
	TFTD_WriteData(0x23); 
	TFTD_WriteData(0x37); 
	TFTD_WriteData(0x00); 	
	TFTD_WriteData(0x07); 
	TFTD_WriteData(0x02); 
	TFTD_WriteData(0x10); 

	TFTD_WriteCmd(0xe1); 
	TFTD_WriteData(0x0f); 
	TFTD_WriteData(0x1b); 
	TFTD_WriteData(0x0f); 
	TFTD_WriteData(0x17); 
	TFTD_WriteData(0x33); 
	TFTD_WriteData(0x2c); 
	TFTD_WriteData(0x29); 
	TFTD_WriteData(0x2e); 
	TFTD_WriteData(0x30); 
	TFTD_WriteData(0x30); 
	TFTD_WriteData(0x39); 
	TFTD_WriteData(0x3f); 
	TFTD_WriteData(0x00); 
	TFTD_WriteData(0x07); 
	TFTD_WriteData(0x03); 
	TFTD_WriteData(0x10);  
	
	TFTD_WriteCmd(0x2a);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x7f);

	TFTD_WriteCmd(0x2b);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x00);
	TFTD_WriteData(0x9f);
	
	TFTD_WriteCmd(0xF0); //Enable test command  
	TFTD_WriteData(0x01); 
	TFTD_WriteCmd(0xF6); //Disable ram power save mode 
	TFTD_WriteData(0x00); 
	
	TFTD_WriteCmd(0x3A); //65k mode 
	TFTD_WriteData(0x05); 
	
	
	TFTD_WriteCmd(0x29);//Display on	 

}







