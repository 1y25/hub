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
	//ADC
#include "A_ADC.h"

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
	Init_ADC();
	
	//进入临界区
	taskENTER_CRITICAL();
		//线程函数-格式建议用Task_Xxx
	xTaskCreate(Task_Func,"Func",64,NULL,1,NULL);
	xTaskCreate(Task_PWM,"PWM",32,NULL,1,NULL);
	vTaskDelay(100);
	xTaskCreate(Task_ShowPic,"ShowPic",128,NULL,9,NULL);
	
	
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
extern int8_t write_sign;
extern uint16_t pic_delay;
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
		U_Printf("需要用Tool中的FlashPic.py烧录图片 \r\n");
		U_Printf("例如:python FlashPic.py ./chaofan/ 3 37 \r\n");
		U_Printf("指，将./chaofan/文件夹中的37张图片烧录到W25Q64的第三个位置上(共4个放图片的位置) \r\n");
		U_Printf("其他指令： \r\n");
		U_Printf("ReadInfo : 检查文件头 \r\n");
		U_Printf("CHPIC-x : 切换图片到第x张 \r\n");
		U_Printf("SetDelay-xx : 更改帧间隔，值越大屏幕刷新越慢 \r\n");
	}
	else if(Command("WritePic"))
	{
		Read_Pic();
	}
	else if(Command("ReadInfo"))
	{
		ReadPicInfo();
		for(int i=1;i<5;i++)
		{
			U_Printf("PIC_INFO[%d]:[%d*%d],frame:%d,pixel_counts:%d \r\n",i,PIC_INFO[i].width,PIC_INFO[i].height,PIC_INFO[i].frame+1,PIC_INFO[i].pixel_count);
		}
		U_Printf("当前显示图片下标: %d \r\n",pic_index);
		U_Printf("图片帧间隔：%d(+50ms)(屏幕刷新占用约50ms) \r\n",pic_delay);
	}
	else if(Command("CHPIC"))
	{
		pic_index = usart1_buff[6]-'0';
		U_Printf("更换到%d张图片 \r\n",pic_index);
		write_sign = 1;
	}
	else if(Command("SetDelay"))
	{
		pic_delay = 10*(usart1_buff[9]-'0');
		pic_delay += (usart1_buff[10]-'0');
		U_Printf("帧间隔更改为:%d \r\n",pic_delay);
		write_sign = 1;
	}
	//结束
	else
	{
		return 0;
	}
	return 1;
}

