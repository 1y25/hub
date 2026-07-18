#ifndef __W_W25QXX_H__
#define __W_W25QXX_H__
#include "stdint.h"

//架构接口
void Init_WQ(void);
void Task_WQ(void* pvParameters);
void Cmd_WQ(void);

//基本操作
void WQ_Start(void);
void WQ_Stop(void);
uint8_t WQ_Swap(uint8_t data);
void WQ_WaitProcess(void);
void WQ_WriteEnable(void);
void WQ_Write(uint32_t addr,uint8_t* bytes,uint16_t length);
void WQ_Read(uint32_t addr,uint8_t* bytes,uint16_t length);

//使用DMA进行操作
void WQ_Erease(uint32_t addr_xxx000);
void WQ_Test(void);
void WQ_WriteStart(uint32_t addr_xxxx00);
void WQ_WriteStop();
void WQ_ReadStart(uint32_t addr_xxxx00);
void WQ_ReadStop(void);

//对ram进行操作
void WQ_RamRead(uint32_t addr,int8_t front);
void WQ_RamWrite(uint32_t addr,int8_t front);

#endif
