#ifndef __TFT_FONT_H__
#define __TFT_FONT_H__
#include "stdint.h"
/*	有关函数命名前缀为UI_ (即TFT_Font)
 */

/*  字体定义  */
typedef struct tft_font{
	const char* font_lib;			//字体库/图片库
	uint8_t 	height;			//单个字体/图片的高度
	uint8_t 	width;			//单个字体/图片的宽度
}tft_font;

/*  关于形状的部分  */
void UI_Draw_Rect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color);
void UI_Draw_Frame(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color,int8_t thick);
void UI_Draw_Line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color,int8_t thick);

/*  关于取模显示的部分  */
void UI_Put_Pic01(uint16_t x,uint16_t y,uint8_t pic01,uint8_t fcolor,uint8_t bcolor);
void UI_Put_Char(uint16_t x,uint16_t y,char _char,uint8_t font,uint8_t fcolor,uint8_t bcolor);

/*  关于文本/数字的部分  */
void UI_Write_Num(uint16_t x,uint16_t y,uint32_t num,uint8_t font,uint8_t fcolor,uint8_t bcolor,int8_t digits);
void UI_Write_String(uint16_t x,uint16_t y,const char* text,uint8_t font,uint8_t fcolor,uint8_t bcolor,int8_t NumOfChar);


#endif
