#ifndef __FUNC_H__
#define __FUNC_H__
#include "stdint.h"

typedef struct PIC_INFOR {
	uint32_t pixel_count;	//像素数量(宽度*高度)
	uint16_t width;			//宽度
	uint16_t height;		//高度
	uint8_t frame;			//帧数
	uint8_t index;			//图片下标
	uint8_t wq_times;		//读取ram的次数
}pic_infor;
void Read_Pic(void);
void Show_Pic(uint8_t pic_index,uint8_t frame_index);
void Init_Func(void);
void Task_Func(void* pvParameters);
void WritePicInfo(void);
void ReadPicInfo(void);
void ShowPic_WithFrame(uint8_t index);
void Task_ShowPic(void* pvParameters);
void Init_UIFrame(void);

#endif
