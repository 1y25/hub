#ifndef __UI_DEF_H__
#define __UI_DEF_H__
#include "stdint.h"

/* 颜色 */
#define COLOR_BLACK		0
#define COLOR_BLUE		1
#define COLOR_WHITE1	2
#define COLOR_PINK		3
#define COLOR_WHITE		4
#define COLOR_NUM		5

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
