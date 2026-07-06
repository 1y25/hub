#ifndef __TFT_DMA_H__
#define __TFT_DMA_H__
#include "stdint.h"

void Init_TFTD(void);
void Task_TFTD(void* pvParameters);
void Cmd_TFTD(void);
void TFTD_SetRect(uint16_t x,uint16_t y,uint16_t width,uint16_t height);
void TFTD_WriteData16(uint16_t rgb565);
void TFTD_Start(void);
void TFTD_Stop(void);

#endif
