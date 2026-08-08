#ifndef __U_USART1_H__
#define __U_USART1_H__
#include <stdint.h>

void U_USART1_Init(void);
void U_DeInitDMA(void);
void U_InitDMA(void);
void U_Putchar(uint8_t _char);
void U_Printf(const char* words,...);
uint8_t Command(const char* command);

#endif
