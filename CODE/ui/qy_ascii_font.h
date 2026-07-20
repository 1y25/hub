#ifndef __QY_ASCII_FONT_H__
#define __QY_ASCII_FONT_H__
//QY是秦羽的缩写

/*	填充方向:+------------>
 *			|			 x
 *			|	1  2  3
 *			|	|  |  |
 *			|	|  |  |
 *			|	V  V  V
 *			v y
 *	PCtoLCD2002中 设置  阴码 逐列式 逆向(低位在前) 
 *				————2025/8/8-16:03
 */
/*	命名是font_ASCII(ascii偏移32位)/PIC(图片)_字体名称_高宽
 */
/*	当前库里有
	//Pixel LCD-7
	//测试发现这个字体的字母只有大写没有小写
const char font_ASCII_PIXEL_2412[][36];
const char font_ASCII_PIXEL_3216[][64];
	//NI7SEG
	//发现这个字体的字母也只有大写
const char font_ASCII_NI7SEG_2412[][36];
const char font_ASCII_NI7SEG_3216[][64];
	//MS Gothic
const char font_ASCII_Gothic_2412[][36];
const char font_ASCII_Gothic_3216[][64];
 */



#endif
