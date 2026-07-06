#include "UI_DEF.h"
#include "stdint.h"

/*  屏幕  */
#include "TFT_DMA.h"
#include "TFT_ST7735.h"
/**@brief  设定显示位置
  *@add    这里宽和高和X/Yend大概是Xend=width+x-1，Yend=height+y-1
  */
void UI_SetRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height)
{	
	TFT_SetRect(x,y,width,height);
}
/**@brief  发送单个像素
  */
void UI_Pixel(uint16_t rgb565)
{
	TFT_WriteData16(rgb565);
}

/*  颜色  */
uint16_t COLOR[32];

/*  字体  */
#include "TFT_font.h"	//引入tft_font
tft_font FONT[10];

/**  颜色初始化  **/
uint16_t UI_RGB(uint32_t rgb888)
{
	uint16_t rgb565 = 0;
	rgb565 = rgb888>>19;
	rgb565 = rgb565<<6;
	rgb565 |= ((rgb888>>10)&0x3F);
	rgb565 = rgb565<<5;
	rgb565 |= ((rgb888>>3)&0x1F);
	
	return rgb565;
}
static void Init_Color(void)
{
	//颜色初始化
	uint32_t colors[16] = {
		0x000000	
		,0x3b2e7e	//藏蓝
		,0x9d2933	//胭脂红
		,0xd9b611	//秋香色
		,0x40de5a	//草绿
		,0x88ada6	//水色
		,0x725e82	//乌色
		,0xf47983	//桃红
		,0xff8936	//橘黄
		,0x0eb83a	//葱青
		,0x70f3ff	//蔚蓝
		,0xff2121	//大红
		,0xeacd76	//金
		,0x75664d	//黎明
		,0xd6ecf0	//月白
	    ,0xFFFFFF	
	};
	for(int i=0;i<16;i++)
	{
		COLOR[i] = UI_RGB(colors[i]);
	}
}

/**  字体初始化  **/
#include "qy_ascii_font.h"
const char font_ASCII_PIXEL_2412[][36];
const char font_ASCII_PIXEL_3216[][64];
const char font_ASCII_NI7SEG_2412[][36];
const char font_ASCII_NI7SEG_3216[][64];
static void FastInit_Font(uint8_t font_index,uint32_t font_lib,uint8_t height,uint8_t width)
{
	FONT[font_index].font_lib = (const char*)font_lib;
	FONT[font_index].width = width;
	FONT[font_index].height = height;
}
static void Init_Font(void)
{
	FastInit_Font(FONT_PIXEL_2412,(uint32_t)font_ASCII_PIXEL_2412,24,12);
	FastInit_Font(FONT_PIXEL_3216,(uint32_t)font_ASCII_PIXEL_3216,32,16);
	FastInit_Font(FONT_NI7SEG_2412,(uint32_t)font_ASCII_NI7SEG_2412,24,12);
	FastInit_Font(FONT_NI7SEG_3216,(uint32_t)font_ASCII_NI7SEG_3216,32,16);
	
}

/**@brief  UI组件初始化
  */
void Init_UI(void)
{
	//字体初始化
	Init_Font();
	//颜色初始化
	Init_Color();
}



