#ifndef __UI_DEF_H__
#define __UI_DEF_H__
#include "stdint.h"

/* 颜色 */
#define COLOR_BLACK 		0
#define COLOR_BLUE			1
#define COLOR_RED			2
#define COLOR_YELLOW		3
#define COLOR_GREEN			4
#define COLOR_WATER			5
#define COLOR_PURPLE		6
#define COLOR_PETCH			7
#define COLOR_ORIANGE		8
#define COLOR_LIGHT_GREEN	9
#define COLOR_DARK_BLUE		10
#define COLOR_DARK_RED 		11
#define COLOR_GOLD			12
#define COLOR_LIGHT_WHITE	13
#define COLOR_MOON			14
#define COLOR_WHITE			15

/* 字体 */
/* 需要引入TFT_font.h */
#define FONT_PIXEL_2412		0
#define FONT_PIXEL_3216		1
#define FONT_NI7SEG_2412	2
#define FONT_NI7SEG_3216	3
#define FONT_PIC_Test		4

/*  初始化  */
void Init_UI(void);
/*  接口  */
void UI_SetRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void UI_Pixel(uint16_t rgb565);
/*  RGB变换  */
uint16_t UI_RGB(uint32_t rgb888);

#endif
