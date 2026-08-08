#include "TFT_font.h"
#include "UI_DEF.h"
#include "stdint.h"

/*	打算脱离屏幕驱动和单片机底层 
 *	来实现一个完全在上层的取模实现库
 *	
 *	通过实现以下接口后即可使用
 *	注意... >>>屏幕显示方向<<< 和字符取模方向需要调整为以下方向
 *	填充方向:+------------>
 *			|			 x
 *			|	1  2  3
 *			|	|  |  |
 *			|	|  |  |
 *			|	V  V  V
 *			v y
 *	（即img2lcd中的 扫描模式:	垂直扫描 字节内像素数据反序）
 *	（以及PCtoLCD2002中 设置  阴码 逐列式 逆向(低位在前) ）
 *	注：以下提到坐标相关都是指 左上角
 *				2025/7/31-9:39
 */
/*	原本tft_font是在这个(TFT_font.h)中定义的
 *	这个文件也是基于tft_font结构体对屏幕函数进行一些显示实现
 *	文件关系重新调整后把tft_font移动到TFT_UI中
 *	但这个文件仍然叫tft_font.h/c
 *			——2025/8/16-15:01.秦羽
 */

extern tft_font FONT[];
extern uint16_t COLOR[];

/*  关于形状的部分  */

/**@brief  填充矩形
  *@param  -
  *@param  color 颜色
  *@retval void
  */
void UI_Draw_Rect(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color)
{
	UI_SetRect(x,y,width,height);
	for(int i=0;i<width*height;i++)
	{
		UI_Pixel(color);
	}
}
/**@brief  绘制矩形框
  *@param  x y width height
  *@param  color 框颜色
  *@param  thick 框线宽
  *@retval void
  *@add    注：框架线是向内收缩的 x y width height是最大外边框
  */
void UI_Draw_Frame(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t color,int8_t thick)
{
	if(thick<=0)
	{
		return;
	}
	UI_Draw_Rect(x,y,width,thick,color);
	UI_Draw_Rect(x,y+height-thick,width,thick,color);
	UI_Draw_Rect(x,y+thick,thick,height-thick*2,color);
	UI_Draw_Rect(x+width-thick,y+thick,thick,height-thick*2,color);
}

/*  关于取模显示的部分  */
/**@brief  显示单张单色图片
  *@param  x,y 坐标(左上角)
  *@param  pic01 图片内容
  */
void UI_Put_01Pic(uint16_t x,uint16_t y,uint8_t pic01,uint8_t fcolor,uint8_t bcolor)
{
	tft_font f_pic01 = FONT[pic01];
	
	UI_SetRect(x,y,f_pic01.width,f_pic01.height);
	uint32_t a=f_pic01.width*f_pic01.height/8;
	for(uint32_t i=0;i<a;i++)
	{
		for(int j=0;j<8;j++)
		{
			if( (f_pic01.font_lib[i]&(0x01<<j)) != 0 )
			{
				UI_Pixel(COLOR[fcolor]);
			}
			else
			{
				UI_Pixel(COLOR[bcolor]);
			}
		}
	}
}
/**@brief  显示图像
  *@param  x,y		位置
  *@param  pic		图像
  */
void UI_Put_565Pic(uint16_t x,uint16_t y,uint8_t font)
{
	tft_font f_font = FONT[font];
	UI_SetRect(x,y,f_font.width,f_font.height);
	for(int i=0;i<f_font.width*f_font.height;i++)
	{
		UI_Pixel(f_font.font_lib[i]);
	}
}
/**@brief  显示单个字符
  *@param  	x,y 	位置
  *@param	_char 	要显示的字符
  *@param  font		字体
  */
void UI_Put_Char(uint16_t x,uint16_t y,char _char,uint8_t font,uint8_t fcolor,uint8_t bcolor)
{
	tft_font f_font = FONT[font];
	UI_SetRect(x,y,f_font.width,f_font.height);
	uint32_t a=f_font.width*f_font.height/8;
	uint16_t index = (_char-' ')*f_font.height*f_font.width/8;
	for(uint32_t i=0;i<a;i++)
	{
		for(int j=0;j<8;j++)
		{
			if( (f_font.font_lib[index+i]&(0x01<<j)) != 0 )
			{
				UI_Pixel(COLOR[fcolor]);
			}
			else
			{
				UI_Pixel(COLOR[bcolor]);
			}
		}
	}
}

/*  关于文本的部分  */

/**@brief  显示一个数字
  *@param  -
  *@param  digits  显示的位数，超过位数会吞掉低位
  */
void UI_Write_Num(uint16_t x,uint16_t y,uint32_t num,uint8_t font,uint8_t fcolor,uint8_t bcolor,int8_t digits)
{
	tft_font f_font = FONT[font];
	uint32_t num_length = 1;
	for(;digits>0;digits--)
	{	
		num_length*=10;
	}
	//从高位开始显示
	int8_t i=0;
	for(num_length/=10;num_length>=1;num_length/=10)
	{
		UI_Put_Char(x+f_font.width*(i++),y,num/num_length+'0',font,fcolor,bcolor);
		//减去最高位
		num -= (num - (num%num_length));
	}
}
/**@brief  字符串
  *@param  -
  *@param  NumOfChar  显示的数量
  */
void UI_Write_String(uint16_t x,uint16_t y,const char* text,uint8_t font,uint8_t fcolor,uint8_t bcolor,int8_t NumOfChar)
{
	tft_font f_font=FONT[font];
	int i=0;
	for(;text[i]!='\0';i++)
	{
		if(--NumOfChar<0)
		{
			UI_Put_Char(x+f_font.width*(--i),y,'-',font,fcolor,bcolor);
			return;
		}
		UI_Put_Char(x+f_font.width*(i),y,text[i],font,fcolor,bcolor);
	}
	for(;NumOfChar>0;NumOfChar--)
	{
		UI_Put_Char(x+f_font.width*(i++),y,' ',font,fcolor,bcolor);
	}
}








