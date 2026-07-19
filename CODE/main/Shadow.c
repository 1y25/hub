#include "Shadow.h"
#include "BaseFunc.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  接口库  */
#include "Func.h"
/*  外设库  */
#include "U_USART1.h"
	//PWM->板子上那个呼吸的粉红色灯
#include "P_PWM.h"
	//TFT屏幕
#include "TFT_DMA.h"
	//W25Q64存储芯片
#include "W_W25QXX.h"

/* 2026/7/17-12:51
 * 写软件驱动好麻烦.....
 * 正在大道至简，把所有初始化都删了
 * 然后慢慢增加功能实现....
 * 现在这个程序是hub.v2的程序...
 *	.秦
 */

/**@brief  初始化线程
  */
void Start_MainTask(void* pvParameters)
{
	//启动内容
	Start_Func();
		//初始化函数-格式建议用Init_Xxx
	Init_Func();
	Init_PWM();
	Init_TFTD();
	Init_WQ();
	ReadPicInfo();
	Show_Pic(0,0);
	
	//进入临界区
	taskENTER_CRITICAL();
		//线程函数-格式建议用Task_Xxx
	xTaskCreate(Task_Func,"Func",64,NULL,1,NULL);
	xTaskCreate(Task_PWM,"PWM",32,NULL,1,NULL);
	
	
	//退出临界区
	taskEXIT_CRITICAL();
	//删除自身函数
	vTaskDelete(NULL);
}

/**@brief  指令监听
  */
extern pic_infor PIC_INFO[];
extern uint8_t pic_index;
uint8_t ram_hub[1024*2];
extern uint8_t usart1_buff[64];
extern int8_t usart1_isbuff;
extern uint16_t usart1_count;
uint8_t Start_CommandFunc(void)
{
	if(Command("Start_CommandFunc"))
	{
		U_Printf("Command(\"COMMAND\")||Command(\"HELP\")\r\n");
	}
	//添加区
	else if(Command("COMMAND")||Command("HELP"))
	{
		U_Printf("这里是stm32f103c8t6的测试程序 \r\n");
		U_Printf("现在在写hubV2相关驱动 \r\n");
	}
	else if(Command("WritePic"))
	{
		Read_Pic();
	}
	else if(Command("ShowPic"))
	{
		PIC_INFO[1].wq_times = 15;
		PIC_INFO[1].width = 160;
		PIC_INFO[1].height = 90;
		PIC_INFO[1].pixel_count = 160*90;
		Show_Pic(1,0);
	}
	else if(Command("ReadInfo"))
	{
		for(int i=0;i<3;i++)
		U_Printf("PIC_INFO[%d]:[%d*%d],frame:%d,pixel_counts:%d \r\n",i,PIC_INFO[i].width,PIC_INFO[i].height,PIC_INFO[i].frame+1,PIC_INFO[i].pixel_count);
		U_Printf("aaaa \r\n");
		ReadPicInfo();
	}
	else if(Command("CHPIC"))
	{
		pic_index = usart1_buff[6]-'1';
		U_Printf("更换到%d张图片 \r\n",pic_index);
	}
	else if(Command("TTT"))
	{
		//进入含DMA的115200传输
		U_InitDMA();
		//等待
		while(usart1_isbuff==0);
		
		//接收到数据
		WQ_Erease(0x22);
		WQ_RamWrite(0x22,1);
		U_Printf("115200传输结束，改回9600 \r\n");
		vTaskDelay(7000);
		
		U_DeInitDMA();
	}
	else if(Command("READ"))
	{
		U_Printf("啊？ \r\n");
		for(int i=0;i<2048;i++)
		{
			ram_hub[i] = 'a';
		}
		WQ_RamRead(0x22,1);
		for(int i=0;i<2048;i++)
		{
			U_Putchar(ram_hub[i]);
		}
	}
	else if(Command("TFT"))
	{
		TFTD_SetRect(20,30,120,68);
				//开始通信
		TFTD_Start();
				//DMA输出处理
		for(int j=0;j<10;j++)
		{
			for(int i=0;i<12*68*2;i++)
			{
				ram_hub[i] = i;
			}
			DMA_Cmd(DMA1_Channel5,DISABLE);
			DMA_SetCurrDataCounter(DMA1_Channel5,12*68);
			DMA_Cmd(DMA1_Channel5,ENABLE);
			while(DMA_GetFlagStatus(DMA1_FLAG_TC5)!=SET);
			DMA_ClearFlag(DMA1_FLAG_TC5);
		}
			//结束通信
		TFTD_Stop();
	}
	//结束
	else
	{
		return 0;
	}
	return 1;
}

