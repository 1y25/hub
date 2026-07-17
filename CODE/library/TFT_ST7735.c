#include "TFT_ST7735.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */ //->仅用于初始化延时
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"

/*	好像入门单片机以来...
 *	一直在做屏幕之类的事情....
 *	就算这么说...也没有...
 *	（其实有能拿得出手的TFT库 但是不想用...）
 *	总之这个项目再之后有时间
 *	打算做电机或者舵机之类的有意思的事情
 *			——2025/5/23-22:10 
 */
/*	当前屏幕方向配置为:
 *	+——————————>x(160)
 *  |	1 2 3 4		*
 *  |	| | | |		*
 *  |	| | | |		*	pin引脚位置
 *  |	v v	v v		*
 *  |	刷新顺序	*
 *  |	可根据"TFT_SoftwareInit"中的提示配置
 *	|	(0x36指令)
 *  v
 *  y(128)
 */
/*  高电平:  GPIOx->BRR  = GPIO_Pin;
 *  低电平:  GPIOx->BSRR = GPIO_Pin;
 *	当前配置为软件 可根据引脚配置成硬件SPI配置
 */
/*	当前在进行>拓展坞v2<的屏幕适配
 *	PB13	->SCK
 *	PB15	->MOSI
 *	PB14	->LED+
 *	PB11	->RST
 *	PB10	->DC
 *	PB12	->CS
 */
//SCL ->PB13
#define PIN_TFT_SCL_High() 	GPIOB->BSRR = GPIO_Pin_13
#define PIN_TFT_SCL_Low() 	GPIOB->BRR  = GPIO_Pin_13
//SDA ->PB15
#define PIN_TFT_SDA_High() 	GPIOB->BSRR = GPIO_Pin_15
#define PIN_TFT_SDA_Low()	GPIOB->BRR  = GPIO_Pin_15
//RST ->PB11
#define PIN_TFT_RST_High()	GPIOB->BSRR = GPIO_Pin_11
#define PIN_TFT_RST_Low()	GPIOB->BRR  = GPIO_Pin_11
//DC  ->PB10
#define PIN_TFT_DC_Data()	GPIOB->BSRR = GPIO_Pin_10
#define PIN_TFT_DC_Cmd()	GPIOB->BRR  = GPIO_Pin_10
//CS  ->PB12
#define PIN_TFT_CS_High()	GPIOB->BSRR = GPIO_Pin_12
#define PIN_TFT_CS_Low()	GPIOB->BRR  = GPIO_Pin_12

/**@brief  内部使用 TFT屏幕相关引脚初始化 日后根据需要修改
  *@param  void
  *@retval void
  */
static void Init_TFTPin(void)
{
	//时钟初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	//引脚初始化
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
	//给初始电平
	GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_SET);
}

/**  以下是接口相关部分  **/

/**@brief  TFT屏幕初始化
  *@param  void
  *@retval void
  */
static void TFT_SoftwareInit(void);
void Init_TFT(void)
{
	Init_TFTPin();
		//CS	  -1(不选中)
	PIN_TFT_CS_High();
		//SCL	  -0(上升沿发送数据)
	PIN_TFT_SCL_Low();
		//RST	  -1(高电平运行)
	PIN_TFT_RST_High();
		//
	vTaskDelay(200);
		//软件初始化
	TFT_SoftwareInit();
		//画蓝白粉()
	uint8_t width = 133/3 +1;
	uint16_t blue  = TFT_RGB888To565(0x5FCDE4);
	uint16_t white = TFT_RGB888To565(0xFFFFFF); 
	uint16_t pink  = TFT_RGB888To565(0xFFB6C1);
	
	TFT_SetRect(0,0,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFT_WriteData16(pink);
	}
	TFT_SetRect(0,width,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFT_WriteData16(white);
	}
	TFT_SetRect(0,width*2,168,width);
	for(int i=0;i<width*168;i++)
	{
		TFT_WriteData16(blue);
	}
	
		//初始化完成
	U_Printf("TFT初始化完成 \r\n");
}
/**@brief  从RGB888转换成RGB565
  *@param  RGB888 颜色代码
  *@retval RGB565颜色代码
  */
uint16_t TFT_RGB888To565(uint32_t RGB888)
{
	uint16_t RGB565 = 0;
	RGB565 = RGB888>>19;
	RGB565 = RGB565<<6;
	RGB565 |= ((RGB888>>10)&0x3F);
	RGB565 = RGB565<<5;
	RGB565 |= ((RGB888>>3)&0x1F);
	return RGB565;
}
/**@brief  TFT发送一条指令
  *@param  cmd  8位数据
  *@retval void
  */
void TFT_WriteCmd(uint8_t cmd)
{
	//指令
	PIN_TFT_DC_Cmd();
	//片选选中
	PIN_TFT_CS_Low();
	//发送八位指令，高位在前
	for(int i=0;i<8;i++)
	{
		if( (cmd&(0x80>>i)) == 0 )
		{
			PIN_TFT_SDA_Low();
		}
		else
		{
			PIN_TFT_SDA_High();
		}
		PIN_TFT_SCL_High();
		PIN_TFT_SCL_Low();
	}
	//片选结束
	PIN_TFT_CS_High();
}
/**@brief  TFT发送一条数据
  *@param  data  8位数据
  *@retval void
  */
void TFT_WriteData(uint8_t data)
{
	//数据
	PIN_TFT_DC_Data();
	//片选选中	
	PIN_TFT_CS_Low();
	//发送八位数据，高位在前
	for(int i=0;i<8;i++)
	{
		if( (data&(0x80>>i)) == 0)
		{
			PIN_TFT_SDA_Low();
		}
		else
		{
			PIN_TFT_SDA_High();
		}
		PIN_TFT_SCL_High();
		PIN_TFT_SCL_Low();
	}
	//片选结束
	PIN_TFT_CS_High();
}
/**@brief  TFT发送一条数据
  *@param  data  16位数据
  *@retval void
  */
void TFT_WriteData16(uint16_t data)
{
	//数据
	PIN_TFT_DC_Data();
	//片选选中	
	PIN_TFT_CS_Low();
	//发送八位数据，高位在前
	for(int i=0;i<16;i++)
	{
		if( (data&(0x8000>>i)) == 0)
		{
			PIN_TFT_SDA_Low();
		}
		else
		{
			PIN_TFT_SDA_High();
		}
		PIN_TFT_SCL_High();
		PIN_TFT_SCL_Low();
	}
	//片选结束
	PIN_TFT_CS_High();

}
/**@brief  设置显示的位置
  *@param  x,y,x_e,y_e 从(x,y)到(x_e,y_e)
  *@retval void
  */
void TFT_SetRect(uint16_t x,uint16_t y,uint16_t width,uint16_t hight)
{
	TFT_WriteCmd(0x2a);
	TFT_WriteData(0x00);
	TFT_WriteData(y);
	TFT_WriteData(0x00);
	TFT_WriteData(y+hight-1);
	
	TFT_WriteCmd(0x2b);
	TFT_WriteData(0x00);
	TFT_WriteData(x);
	TFT_WriteData(0x00);
	TFT_WriteData(x+width-1);
	
	TFT_WriteCmd(0x2c);
}
/**@brief  清屏
  *@param  RGB 565
  *@retval void
  */
void TFT_Clear(uint16_t RGB)
{
	TFT_SetRect(0,0,180,140);
	for(int i=0;i<180*140;i++)
	{
		TFT_WriteData16(RGB);
	}
}
/**@brief  xy轴方向测试
  *@param  void
  *@retval void
  *		+————————>x
  *		|红  黄
  *		|蓝  绿
  *		|
  *     v
  *		y
  */
void Cmd_TFT_XYTest(void)
{
	uint16_t red	=TFT_RGB888To565(0xFFC0CB);
	uint16_t yellow	=TFT_RGB888To565(0xE6D933);
	uint16_t blue	=TFT_RGB888To565(0x87CEEB);
	uint16_t green	=TFT_RGB888To565(0x00FF80);
	TFT_SetRect(0,0,30,30);
	for(int i=0;i<30*30;i++)
	{
		TFT_WriteData16(red);
	}
	TFT_SetRect(30,0,30,30);
	for(int i=0;i<30*30;i++)
	{
		TFT_WriteData16(yellow);
	}
	TFT_SetRect(0,30,30,30);
	for(int i=0;i<30*30;i++)
	{
		TFT_WriteData16(blue);
	}
	TFT_SetRect(30,30,30,30);
	for(int i=0;i<30*30;i++)
	{
		TFT_WriteData16(green);
	}
}
/**@brief  这段完全来自商家的例程
  *@param  void
  *@retval void
  */
static void TFT_SoftwareInit(void)
{
	//Reset before LCD Init.
	PIN_TFT_RST_Low();
	vTaskDelay(100);
	PIN_TFT_RST_High();
	vTaskDelay(50);

	//LCD Init For 1.44Inch LCD Panel with ST7735R.
	TFT_WriteCmd(0x11);//Sleep exit 
	vTaskDelay(120);
		
	//ST7735R Frame Rate
	TFT_WriteCmd(0xB1); 
	TFT_WriteData(0x01); 
	TFT_WriteData(0x2C); 
	TFT_WriteData(0x2D); 

	TFT_WriteCmd(0xB2); 
	TFT_WriteData(0x01); 
	TFT_WriteData(0x2C); 
	TFT_WriteData(0x2D); 

	TFT_WriteCmd(0xB3); 
	TFT_WriteData(0x01); 
	TFT_WriteData(0x2C); 
	TFT_WriteData(0x2D); 
	TFT_WriteData(0x01); 
	TFT_WriteData(0x2C); 
	TFT_WriteData(0x2D); 
	
	TFT_WriteCmd(0xB4); //Column inversion 
	TFT_WriteData(0x07); 
	
	//ST7735R Power Sequence
	TFT_WriteCmd(0xC0); 
	TFT_WriteData(0xA2); 
	TFT_WriteData(0x02); 
	TFT_WriteData(0x84); 
	TFT_WriteCmd(0xC1); 
	TFT_WriteData(0xC5); 

	TFT_WriteCmd(0xC2); 
	TFT_WriteData(0x0A); 
	TFT_WriteData(0x00); 

	TFT_WriteCmd(0xC3); 
	TFT_WriteData(0x8A); 
	TFT_WriteData(0x2A); 
	TFT_WriteCmd(0xC4); 
	TFT_WriteData(0x8A); 
	TFT_WriteData(0xEE); 
	
	TFT_WriteCmd(0xC5); //VCOM 
	TFT_WriteData(0x0E); 
	
	//Y反转-X反转-XY调换-Y刷新方向-RGB(0)/BGR(1)-X刷新方向-0-0
	//0xC0(1100 0000)->(Y反转-X反转-XY不调换-0 0000)
	//0110 0000
	TFT_WriteCmd(0x36); //MX, MY, RGB mode 
	TFT_WriteData(0x80); //1000 0000
	
	//ST7735R Gamma Sequence
	TFT_WriteCmd(0xe0); 
	TFT_WriteData(0x0f); 
	TFT_WriteData(0x1a); 
	TFT_WriteData(0x0f); 
	TFT_WriteData(0x18); 
	TFT_WriteData(0x2f); 
	TFT_WriteData(0x28); 
	TFT_WriteData(0x20); 
	TFT_WriteData(0x22); 
	TFT_WriteData(0x1f); 
	TFT_WriteData(0x1b); 
	TFT_WriteData(0x23); 
	TFT_WriteData(0x37); 
	TFT_WriteData(0x00); 	
	TFT_WriteData(0x07); 
	TFT_WriteData(0x02); 
	TFT_WriteData(0x10); 

	TFT_WriteCmd(0xe1); 
	TFT_WriteData(0x0f); 
	TFT_WriteData(0x1b); 
	TFT_WriteData(0x0f); 
	TFT_WriteData(0x17); 
	TFT_WriteData(0x33); 
	TFT_WriteData(0x2c); 
	TFT_WriteData(0x29); 
	TFT_WriteData(0x2e); 
	TFT_WriteData(0x30); 
	TFT_WriteData(0x30); 
	TFT_WriteData(0x39); 
	TFT_WriteData(0x3f); 
	TFT_WriteData(0x00); 
	TFT_WriteData(0x07); 
	TFT_WriteData(0x03); 
	TFT_WriteData(0x10);  
	
	TFT_WriteCmd(0x2a);
	TFT_WriteData(0x00);
	TFT_WriteData(0x00);
	TFT_WriteData(0x00);
	TFT_WriteData(0x7f);

	TFT_WriteCmd(0x2b);
	TFT_WriteData(0x00);
	TFT_WriteData(0x00);
	TFT_WriteData(0x00);
	TFT_WriteData(0x9f);
	
	TFT_WriteCmd(0xF0); //Enable test command  
	TFT_WriteData(0x01); 
	TFT_WriteCmd(0xF6); //Disable ram power save mode 
	TFT_WriteData(0x00); 
	
	TFT_WriteCmd(0x3A); //65k mode 
	TFT_WriteData(0x05); 
	
	
	TFT_WriteCmd(0x29);//Display on	 
}
