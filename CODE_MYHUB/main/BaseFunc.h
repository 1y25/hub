#ifndef __BASEFUNC_H__
#define __BASEFUNC_H__
#include <stdint.h>

void Start_Func(void);
void Start_LEDInit(void);
void Start_WDGInit(uint16_t wdg_ms);
void Start_LEDTask(void* pvParameters);
void Start_CommandInit(void);
void Start_CommandTask(void* pvParameters);

#endif
