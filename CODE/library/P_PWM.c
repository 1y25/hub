#include "P_PWM.h"
/*  ST库  */
#include "stm32f10x.h"
/*  OS库  */
#include "FreeRTOS.h"
#include "task.h"
/*  外设库  */
#include "U_USART1.h"

/**@brief  PWM初始化
  */
#define PWM_RCCX	RCC_APB2Periph_GPIOB
#define PWM_RCCTIM	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE)
#define PWM_GPIOX	GPIOB
#define PWM_PIN		GPIO_Pin_15
#define PWM_TIMX	TIM1
#define PWM_CHX		3
void Init_PWM(void)
{
	//时钟初始化
	RCC_APB2PeriphClockCmd(PWM_RCCX,ENABLE);
	PWM_RCCTIM;
	//引脚初始化
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin = PWM_PIN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(PWM_GPIOX,&GPIO_InitStruct);
	GPIO_WriteBit(PWM_GPIOX,PWM_PIN,Bit_SET);//高电平亮
	
	//定时器初始化
	TIM_InternalClockConfig(PWM_TIMX);
	TIM_ARRPreloadConfig(PWM_TIMX,ENABLE);
	TIM_TimeBaseInitTypeDef TIM_InitStruct;
	TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_InitStruct.TIM_Period = 1000-1;
	TIM_InitStruct.TIM_Prescaler = 9-1;
	TIM_InitStruct.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(PWM_TIMX,&TIM_InitStruct);
	//PWM初始化
	TIM_CtrlPWMOutputs(PWM_TIMX,ENABLE);
	TIM_OCInitTypeDef PWM_InitStruct;
	PWM_InitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	PWM_InitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	PWM_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
	PWM_InitStruct.TIM_Pulse = 9;
	PWM_InitStruct.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
	PWM_InitStruct.TIM_OCNPolarity = TIM_OCNPolarity_Low;
	PWM_InitStruct.TIM_OutputNState = TIM_OutputNState_Enable;
	PWM_InitStruct.TIM_OCIdleState = TIM_OCIdleState_Reset;
	switch(PWM_CHX)
	{
	case 1:TIM_OC1Init(PWM_TIMX,&PWM_InitStruct);break;
	case 2:TIM_OC2Init(PWM_TIMX,&PWM_InitStruct);break;
	case 3:TIM_OC3Init(PWM_TIMX,&PWM_InitStruct);break;
	case 4:TIM_OC4Init(PWM_TIMX,&PWM_InitStruct);break;
	}
	TIM_Cmd(PWM_TIMX,ENABLE);
	
	U_Printf("PWM初始化完成 \r\n");
}
/**@brief PWM线程
  */
void Task_PWM(void* pvParameters)
{
	int16_t pwm_pulse = 10;
	int16_t position = 1;
	while(1)
	{
		vTaskDelay(15);
		switch(PWM_CHX)
		{
		case 1:TIM_SetCompare1(PWM_TIMX,pwm_pulse);break;
		case 2:TIM_SetCompare2(PWM_TIMX,pwm_pulse);break;
		case 3:TIM_SetCompare3(PWM_TIMX,pwm_pulse);break;
		case 4:TIM_SetCompare4(PWM_TIMX,pwm_pulse);break;
		}
		pwm_pulse += position;
		if(pwm_pulse>=100)
		{
			vTaskDelay(1000);
			position = -1;
		}
		else if(pwm_pulse<=2)
		{
			vTaskDelay(1000);
			position = 1;
		}
	}
}










