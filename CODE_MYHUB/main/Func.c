#include "Func.h"
#include "stm32f10x.h"                  // Device header
#include "FreeRTOS.h"
#include "task.h"
#include "U_USART1.h"
#include "W_W25QXX.h"
#include "TFT_DMA.h"

pic_infor PIC_INFO[5];
uint8_t pic_index = 1;
int8_t write_sign = 0;
int16_t pic_delay = 50;
extern uint8_t ram_hub[1024*2];
extern uint8_t usart1_buff[64];
extern int8_t usart1_isbuff;
extern uint16_t usart1_count;
/* 纯中断收包(见U_USART1.c), 烧录时收数据包用 */
extern volatile uint8_t* rx_ptr;
extern volatile uint16_t rx_count;
extern volatile uint8_t rx_done;
extern volatile uint16_t rx_packet_size;
/* 烧录图片时置1,暂停动画播放,避免读写W25Q64的DMA通道冲突 */
static volatile uint8_t pic_busy = 0;
/* 息屏标志: 长按PA2进入(关背光+停动画), 短按任意键恢复 */
volatile uint8_t screen_off = 0;
/* 每帧占用的4KB扇区数(160x128全屏=20包=80KB, 每图库2MB最多25帧, 帧与帧不重叠) */
#define PIC_FRAME_SECTORS 20

/* ================= 按键(PA1调帧间隔, PA2切换图库, 按下接地=低电平) ================= */
#define BTN_PA1_DOWN() (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)==Bit_RESET)
#define BTN_PA2_DOWN() (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_2)==Bit_RESET)
/**@brief  按键初始化(PA1/PA2 内部上拉输入)
  */
void Init_Button(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef g;
	g.GPIO_Mode = GPIO_Mode_IPU;
	g.GPIO_Speed = GPIO_Speed_2MHz;
	g.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2;
	GPIO_Init(GPIOA,&g);
}
/**@brief  按键扫描任务: PA1循环调帧间隔, PA2短按切图库/长按息屏
  */
void Task_Button(void* pvParameters)
{
	uint8_t last1 = 0;
	uint8_t last2 = 0;
	uint32_t press_ms = 0;		//PA2按住时长(ms)
	uint8_t long_pressed = 0;	//本次按下是否已触发长按
	while(1)
	{
		vTaskDelay(20);
		//烧录图片期间屏蔽按键(避免与写W25Q64冲突)
		if(pic_busy!=0 && !screen_off)
		{
			last1 = BTN_PA1_DOWN();
			last2 = BTN_PA2_DOWN();
			press_ms = 0;
			continue;
		}
		uint8_t b1 = BTN_PA1_DOWN();
		uint8_t b2 = BTN_PA2_DOWN();

		//息屏状态: 短按任意键恢复
		if(screen_off)
		{
			if((b1&&!last1)||(b2&&!last2))
			{
				screen_off = 0;
				GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_SET);	//开背光
				pic_busy = 0;	//恢复动画
				U_Printf("已恢复 \r\n");
			}
			last1 = b1;
			last2 = b2;
			continue;
		}

		//PA2: 长按>=2秒息屏; 短按(<2s)切图库
		if(b2)
		{
			press_ms += 20;
			if(press_ms>=2000 && !long_pressed)
			{
				long_pressed = 1;
				screen_off = 1;
				GPIO_WriteBit(GPIOB,GPIO_Pin_14,Bit_RESET);	//关背光
				pic_busy = 1;	//停动画
				U_Printf("已息屏 \r\n");
			}
		}
		else
		{
			if(press_ms>0 && press_ms<2000 && !long_pressed)
			{
				//PA2短按: 切换图库(1->2->3->4->1)
				pic_index = pic_index%4 + 1;
				write_sign = 1;
				U_Printf("切到图库%d \r\n",pic_index);
			}
			press_ms = 0;
			long_pressed = 0;
		}

		//PA1: 按下沿 → 帧间隔换档(10/25/50/100/200循环)
		if(b1 && !last1)
		{
			if(pic_delay==10)      pic_delay = 25;
			else if(pic_delay==25) pic_delay = 50;
			else if(pic_delay==50) pic_delay = 100;
			else if(pic_delay==100) pic_delay = 200;
			else                   pic_delay = 10;
			write_sign = 1;
			U_Printf("帧间隔: %d \r\n",pic_delay);
		}
		last1 = b1;
		last2 = b2;
	}
}

/* ================= LED呼吸灯(PC13, TIM3中断软件PWM, 1.5k限流电阻) ================= */
/* 呼吸灯状态(中断中读写) */
static volatile uint16_t breath_tick = 0;
static volatile uint8_t breath_duty = 30;
static volatile int8_t breath_dir = 1;
/**@brief  呼吸灯初始化: PC13推挽 + TIM3 1ms更新中断(软件PWM, 不受任务调度影响)
  */
void Init_BreathLED(void)
{
	//PC13引脚
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef g;
	g.GPIO_Mode = GPIO_Mode_Out_PP;
	g.GPIO_Speed = GPIO_Speed_2MHz;
	g.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOC,&g);
	GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
	//TIM3: 10kHz更新中断(0.1ms/tick) -> PWM周期10ms内100 tick, 真·100级亮度
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	TIM_TimeBaseInitTypeDef tb;
	tb.TIM_Period = 899;        // 900 tick @ 8分频 -> 10kHz
	tb.TIM_Prescaler = 7;       // 72MHz/8 = 9MHz
	tb.TIM_ClockDivision = TIM_CKD_DIV1;
	tb.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3,&tb);
	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel = TIM3_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 3;
	nvic.NVIC_IRQChannelSubPriority = 3;
	NVIC_Init(&nvic);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM3,ENABLE);
}
/**@brief  TIM3中断: 1ms/次, 10ms PWM周期(100Hz), 占空比breath_duty/100渐变
  *@note   LED接法默认PC13->1.5k->LED->GND(高电平亮); 若LED另一侧接3.3V, 把SET/RESET对调
  */
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)
	{
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
		//PWM周期100 tick(10ms, 100Hz), 占空比breath_duty/100 (1%步进)
		if((breath_tick%100) < breath_duty)
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_SET);
		}
		else
		{
			GPIO_WriteBit(GPIOC,GPIO_Pin_13,Bit_RESET);
		}
		breath_tick++;
		//每300 tick(30ms)变一级, 100级=3s单程, 约6s一呼吸
		if((breath_tick%300)==0)
		{
			breath_duty += breath_dir;
			if(breath_duty>=100){ breath_duty=100; breath_dir=-1; }
			if(breath_duty==0){ breath_dir=1; }
		}
	}
}
/**@brief  测试函数初始化
  */
void Init_Func(void)
{
	U_Printf("Func初始化完成 \r\n");
}
/**@brief  测试线程
  */
void Task_Func(void* pvParameters)
{
	while(1)
	{
		vTaskDelay(90);
	}
}
/**@breif  显示ADC的框架
  */
#include "UI_DEF.h"
extern uint16_t COLOR[];
void Init_UIFrame(void)
{
	uint8_t adc_y = 103;
	uint8_t Frame_color = COLOR_WHITE;
	UI_Draw_Frame(1,100,160,30,COLOR[Frame_color],3);
	UI_Draw_Rect(54,103,2,24,  COLOR[Frame_color]);
	UI_Draw_Rect(106,103,2,24, COLOR[Frame_color]);
}
/**@brief  多帧动画连续显示，通常设置0.1s切换一张图片
  *@param  index  显示动画的下标，涉及在W25Q64的读取位置
  */
void ShowPic_WithFrame(uint8_t index)
{
	uint16_t frame = PIC_INFO[index].frame;
	for(int i=0;i<=frame;i++)
	{
		Show_Pic(index,i);
		vTaskDelay(pic_delay-2);
		if(write_sign!=0)
		{
			return;
		}
	}
}
/**@brief  显示动画的线程
  */
void Task_ShowPic(void* pvParameters)
{
	while(1)
	{
		if(pic_busy!=0)
		{
			//烧录图片期间暂停动画,避免和W25Q64读写冲突
			vTaskDelay(20);
			continue;
		}
		ShowPic_WithFrame(pic_index);
		if(write_sign!=0)
		{
			write_sign = 0;
			WritePicInfo();
		}
	}
}
/**@brief  从串口写入一帧图片，需要命令字和python程序配合
  */
uint16_t param_a[5];
void Read_Pic(void)
{
	//WritePic-xxx-xxx-xxx-xxx-xxx	//传输次数-长-宽-第几张图片-第几帧
	for(int i=0;i<28;i++)
	{
		usart1_buff[i]-='0';
	}
	param_a[0] = usart1_buff[9]*100;	//次数
	param_a[0] += usart1_buff[10]*10;
	param_a[0] += usart1_buff[11];
	param_a[1] = usart1_buff[13]*100;	//宽
	param_a[1] += usart1_buff[14]*10;
	param_a[1] += usart1_buff[15];
	param_a[2] = usart1_buff[17]*100;	//高
	param_a[2] += usart1_buff[18]*10;
	param_a[2] += usart1_buff[19];
	param_a[3] = usart1_buff[21]*100;	//第几张
	param_a[3] += usart1_buff[22]*10;
	param_a[3] += usart1_buff[23];
	param_a[4] = usart1_buff[25]*100;	//第几帧
	param_a[4] += usart1_buff[26]*10;
	param_a[4] += usart1_buff[27];
//	U_Printf("次数:%d \t长:%d 宽:%d\t第%d张第%d帧 \r\n",param_a[0],param_a[1],param_a[2],param_a[3],param_a[4]);
	param_a[4] -= 1;
	uint8_t index = param_a[3];
	PIC_INFO[index].wq_times = param_a[0];
	PIC_INFO[index].width = param_a[1];
	PIC_INFO[index].height = param_a[2];
	PIC_INFO[index].pixel_count = param_a[1]*param_a[2];
	PIC_INFO[index].frame = param_a[4];
	
	uint8_t pic_index = index;
	pic_index *= 2;
	pic_index -= 1;
	uint16_t addr = (pic_index<<8)+(param_a[4]*PIC_FRAME_SECTORS);
	
	
	//进入烧录状态,暂停动画播放,防止占用W25Q64的DMA通道
	pic_busy = 1;
	U_Printf("ReadyToReadPic \r\n");
	vTaskDelay(300);
	U_InitDMA();	//把USART初始化为115200
	vTaskDelay(300);
	for(int i=0;i<param_a[0];i++)
	{
		WQ_Erease(addr+i);
		U_Printf("E%d\r\n",i);	//诊断: 打印擦除进度(115200, flashpic会显示)
	}
	for(int i=0;i<param_a[0];i++)
	{
		//纯中断收包(不用DMA, 不与TFT刷屏抢DMA1_CH5)
		rx_ptr = &ram_hub[0];
		rx_count = 0;
		rx_done = 0;
		rx_packet_size = 2048;
		U_Printf("NEXT \r\n");	
		while(rx_done==0);	//中断逐字节收满2048或空闲结束
		U_Printf("R%d\r\n",i);	//诊断: 第i包已收到(2KB)
		WQ_RamWrite(addr+i/2,i%2);
		U_Printf("W%d\r\n",i);	//诊断: 第i包已写入W25Q64
	}
	U_Printf("FINISH \r\n");
	vTaskDelay(100);
	U_DeInitDMA();
	
//	ShowPic_WithFrame(index);
	Show_Pic(index,param_a[4]);
	WritePicInfo();
	//烧录与收尾全部完成后才恢复动画, 避免动画任务与上面的Show_Pic抢W25Q64的DMA通道
	pic_busy = 0;
}
/**@brief  显示单张图片
  */
void Show_Pic(uint8_t pic_index,uint8_t frame_index)
{
	uint16_t temp_addr = pic_index*2;
	temp_addr -= 1;
	uint16_t addr = (temp_addr<<8)+(frame_index*PIC_FRAME_SECTORS);
	uint32_t pixel_counts = PIC_INFO[pic_index].pixel_count;
	//居中显示: 基准用屏幕显示区(162x130, 参考项目原值), 图162x130正好(0,0)盖满
	int16_t x = (162-PIC_INFO[pic_index].width)/2;
	int16_t y = (130-PIC_INFO[pic_index].height)/2;
	if(x<0) x = 0;
	if(y<0) y = 0;
//	uint16_t x=0,y=0;
	TFTD_SetRect(x,y,PIC_INFO[pic_index].width,PIC_INFO[pic_index].height);
	TFTD_Start();
	for(int i=0;i<PIC_INFO[pic_index].wq_times;i++)
	{
		//读取W25Q64
		WQ_RamRead(addr+i/2,i%2);
		//软件SPI刷屏(每次最多1024个16位像素,即一个ram_hub周期)
		uint16_t chunk = 1024;
		if(pixel_counts<1024)
		{
			chunk = (uint16_t)pixel_counts;
		}
		TFTD_WriteHalfWords((uint16_t*)&ram_hub[0],chunk);
		pixel_counts -= chunk;
	}
	TFTD_Stop();
}
/**@brief  读取存在W25Q64中的图片数据
  */
void ReadPicInfo(void)
{
	int i=0;
	WQ_RamRead(0,0);
	uint32_t* temp_ram = (uint32_t*)&ram_hub[0];
	for(;i<5;i++)
	{
		PIC_INFO[i].frame			=	temp_ram[i*10+0];
		PIC_INFO[i].height			=   temp_ram[i*10+1];
		PIC_INFO[i].index			=   temp_ram[i*10+2];
		PIC_INFO[i].pixel_count		=   temp_ram[i*10+3];
		PIC_INFO[i].width			=   temp_ram[i*10+4];
		PIC_INFO[i].wq_times		=   temp_ram[i*10+5];
		if(i==0)
		{
			continue;
		}
//		U_Printf("PIC_INFO[%d]:[%d*%d],frame:%d,pixel_counts:%d \r\n",i,PIC_INFO[i].width,PIC_INFO[i].height,PIC_INFO[i].frame+1,PIC_INFO[i].pixel_count);
	}
	pic_index = temp_ram[++i];
	pic_delay = temp_ram[++i];
	U_Printf("W25Q64读取文件头 \r\n");
}
/**@brief  在W25Q64的0x000000写入图片数据
  */
void WritePicInfo(void)
{
	int i=0;
	WQ_Erease(0);
	uint32_t* temp_ram = (uint32_t*)&ram_hub[0];
	for(i=0;i<5;i++)
	{
		temp_ram[i*10+0] = PIC_INFO[i].frame;
		temp_ram[i*10+1] = PIC_INFO[i].height;
		temp_ram[i*10+2] = PIC_INFO[i].index;
		temp_ram[i*10+3] = PIC_INFO[i].pixel_count;
		temp_ram[i*10+4] = PIC_INFO[i].width;
		temp_ram[i*10+5] = PIC_INFO[i].wq_times;
	}
	temp_ram[++i] = pic_index;
	temp_ram[++i] = pic_delay;
	WQ_RamWrite(0,0);
	U_Printf("W25Q64已更新文件头 \r\n");
}




