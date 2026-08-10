#include "TFT_DMA.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"
/*  RGB565转换函数声明  */
#include "TFT_ST7735.h"

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
 *	★ 2026 新板适配（v4·最终，自动探测确认组3）
 *	PB13	->SCK   (SPI2_SCK 硬件复用)
 *	PB15	->MOSI  (SPI2_MOSI 硬件复用)
 *	PB14	->LED+  背光(高电平亮)
 *	PB7	->RST
 *	PB6	->DC
 *	PB5	->CS
 */

//RST ->PB7
#define PIN_TFTD_RST_High()	GPIOB->BSRR = GPIO_Pin_7
#define PIN_TFTD_RST_Low()	GPIOB->BRR  = GPIO_Pin_7
//DC  ->PB6
#define PIN_TFTD_DC_Data()	GPIOB->BSRR = GPIO_Pin_6
#define PIN_TFTD_DC_Cmd()	GPIOB->BRR  = GPIO_Pin_6
//CS  ->PB5
#define PIN_TFTD_CS_High()	GPIOB->BSRR = GPIO_Pin_5
#define PIN_TFTD_CS_Low()	GPIOB->BRR  = GPIO_Pin_5

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
		//其他引脚(RST=PB7 DC=PB6 CS=PB5 背光=PB14)
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_14;
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
	//预分频2 -> 18MHz(排查期降过频, 现已确认接线无问题, 恢复高速刷屏)
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
		//删除三色条测试, 上电黑屏, 动画任务启动后直接全屏播放(无残留)
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
	//标准映射(CASET=X, RASET=Y) + 面板可视区偏移
	//1号板实测: 偏移(1,2)会让左上空1px, 改(0,0)贴满; 若边缘花屏再微调
	#define TFT_PANEL_XOFF 0
	#define TFT_PANEL_YOFF 0
	TFTD_WriteCmd(0x2a);
	TFTD_WriteData16(x+TFT_PANEL_XOFF);
	TFTD_WriteData16(x+TFT_PANEL_XOFF+width-1);

	TFTD_WriteCmd(0x2b);
	TFTD_WriteData16(y+TFT_PANEL_YOFF);
	TFTD_WriteData16(y+TFT_PANEL_YOFF+height-1);

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
/**@brief  连续写多个16位像素(动画刷屏用, DMA1_CH5把ram_hub灌进SPI2)
  *@param  buf    像素缓冲区(小端RGB565, 与W25Q64中存储一致)
  *@param  count  16位像素个数(一次不超过1024)
  *@retval void
  *@note   CS保持低不切换(由TFTD_Start/Stop控制整帧), 避免块间断流产生横纹
  */
void TFTD_WriteHalfWords(const uint16_t* buf, uint32_t count)
{
	//数据
	PIN_TFTD_DC_Data();
	if(count>1024)
	{
		count = 1024;
	}
	//重新指向本次缓冲区
	DMA_Cmd(DMA1_Channel5,DISABLE);
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = count;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)buf;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&SPI2->DR;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel5,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel5,ENABLE);
	DMA_ClearFlag(DMA1_FLAG_TC5);
	while(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=SET);
	DMA_ClearFlag(DMA1_FLAG_TC5);
	//不拉高CS, 保持整帧连续(避免横纹)
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
	//★ MV=1横屏 + 左右镜像(实测绿左上红右上) → MX翻为0 → 0x60
	TFTD_WriteCmd(0x36); //MX, MY, RGB mode
	TFTD_WriteData(0x60); //0110 0000 (MV=1 MY=1 MX=0)
	
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







