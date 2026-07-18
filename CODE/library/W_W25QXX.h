#ifndef __W_W25QXX_H__
#define __W_W25QXX_H__
#include "stdint.h"

void Init_WQ(void);
void Task_WQ(void* pvParameters);
void Cmd_WQ(void);
void WQ_Start(void);
void WQ_Stop(void);
uint8_t WQ_Swap(uint8_t data);

#endif
