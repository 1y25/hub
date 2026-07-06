#include "stm32f10x.h"                  // Device header

uint8_t usart1_buff[256];
uint16_t usart1_count = 0;
int8_t usart1_isbuff = 0;

void U_USART1_Init(void)
{
	//时钟初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIO_InitStruct;
	//引脚初始化
		//PA9->USART1_TX
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
		//PA10->USART1_RX
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);
	//外设初始化
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 9600;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStruct);
	//启用USART1
	USART_Cmd(USART1,ENABLE);
	//DMA初始化 (USART1->DMA1_Channel5)
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);
	DMA_InitTypeDef DMA_InitStruct;
	DMA_InitStruct.DMA_BufferSize = 254;
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
	DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)usart1_buff;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_Init(DMA1_Channel5,&DMA_InitStruct);
	DMA_Cmd(DMA1_Channel5,ENABLE);
	//中断初始化
	NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 5;
	NVIC_Init(&NVIC_InitStruct);
	//设置空闲中断
	USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);
}

extern int8_t wq_trans;
extern int8_t wq_usart;
void USART1_IRQHandler(void)
{
	
	if(USART_GetITStatus(USART1,USART_IT_IDLE)==SET)
	{
		if(!wq_trans)
		{
			//获取数据数量(254->DMA_BufferSize)
			usart1_count = 254 - DMA_GetCurrDataCounter(DMA1_Channel5);
			usart1_buff[usart1_count++] = '\0';
			//重装DMA
			DMA_Cmd(DMA1_Channel5,DISABLE);
			DMA_SetCurrDataCounter(DMA1_Channel5,254);
			DMA_Cmd(DMA1_Channel5,ENABLE);
			//置标志位
			usart1_isbuff = 1;
		}
		else
		{
			wq_usart = 1;
		}
		//清除IDLE标志位
		USART1->SR;
		USART1->DR;
	}
}

void U_Putchar(uint8_t _char)
{
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)!=SET);
	USART_SendData(USART1,(uint16_t)_char);
}

/*  下方是U_Printf实现部分，耦合性很低（没有），可移植  */

/**@brief  发送一段文本
  *@param  words 要发送的文本
  *@retval void
  */
void U_SendWords(const char* words)
{
	for(uint16_t i=0;words[i]!='\0';i++)
	{
		U_Putchar(words[i]);
	}
}
/**@brief  发送一个整数
  *@param  num 要发送的数字
  *@retval void
  */
void U_SendNum(uint32_t num)
{
	if(num==0)
	{
		U_Putchar('0');
		return;
	}
	uint32_t num_length = 1;
	//测量数字位数
	for(;num_length<=num;num_length*=10);
	//从高位开始发送
	for(num_length/=10;num_length>=1;num_length/=10)
	{
		U_Putchar( num/num_length +'0');
		//减去最高位
		num -= (num - (num%num_length));
	}
}
/**@brief  把hex转成字符
  *@param  hex 要转的十六进制
  *@retval 字符
  *@add    内容太敷衍了 但是这个函数又不能没有...
  */
char HexToChar(uint8_t hex)
{
	if(hex<10)
	{
		return hex+'0';
	}
	else 
	{
		return hex+'A'-10;
	}
}
/**@brief  发送一个十六进制
  *@param  hex 要发送的十六进制
  *@retval void
  */
void U_SendHex(uint32_t hex)
{
	U_SendWords("0x");
	if(hex==0)
	{
		U_Putchar('0');
		return;
	}
	//测量位数
	uint32_t hex_length = 0x01;
	int length_num = 0;
	for(;hex_length<=hex;hex_length = hex_length<<1)
	{
		length_num++;
	}
	while(length_num%4!=0)
	{
		length_num++;
		hex_length = hex_length<<1;
	}
	length_num /= 4;
	length_num--;
	//从高位发送数字
	for(;length_num>=0;length_num--)
	{
		U_Putchar(HexToChar( (( hex>>(length_num*4) )&0xF) ));
	}
}
/**@brief  发送二进制
  *@param  binary 要发送的二进制
  *@retval void
  */
void U_SendBinary(uint32_t binary)
{
	U_SendWords("0b");
	if(binary==0)
	{
		U_Putchar('0');
		return;
	}
	//测量长度
	uint32_t length = 0x1;
	uint32_t length_num=0;
	for(;length<=binary;length=length<<1)
	{
		length_num++;
	}
	while(length_num%4!=0)
	{
		length_num++;
		length=length<<1;
	}
	length=length>>1;
	for(;length>=0x01;length=length>>1)
	{
		if(length_num%4==0)
		{
			U_Putchar(' ');
		}
		if( (binary&length) == 0 )
		{
			U_Putchar('0');
		}
		else 
		{
			U_Putchar('1');
		}
		length_num--;
	}
}
/**@brief  printf
  *@param  ...
  *@retval void
  */
#include "stdarg.h"
void U_Printf(const char* words,...)
{
	va_list ap;
	va_start(ap,words);
	for(int i=0;words[i]!='\0';i++)
	{
		if(words[i]=='%')
		{
			switch(words[++i])
			{
			//整数
			case 'D':case 'd':U_SendNum(va_arg(ap,uint32_t));break;
			//字符串
			case 'S':case 's':U_SendWords(va_arg(ap,char*));break;
			//单个字符
			case 'C':case 'c':U_Putchar(va_arg(ap,int));break;
			//十六进制，建议使用标准库X
			case 'H':case 'h':case 'X':case 'x':U_SendHex(va_arg(ap,uint32_t));break;
			//二进制
			case 'B':case 'b':U_SendBinary(va_arg(ap,uint32_t));break;
			}
			continue;
		}
		U_Putchar(words[i]);
	}
	va_end(ap);
}
/**@brief  测试字符串是否匹配
  *@param  command 指令
  *@retval 0不匹配 1匹配
  */
uint8_t Command(const char* command)
{
	uint8_t i=0;
	for(i=0;command[i]!='\0';i++)
	{
		if(i==255)
		{
			U_Printf("M_USART2.c异常，指令过长\r\n");
			return 0;
		}
		if(command[i]!=usart1_buff[i])
		{
			return 0;
		}
	}
//	现在用USART2_Signal标志位，不删指令
//	USART2_Buf[i-1]=' ';
//	@add    匹配后会删除串口缓冲区指令最后一个字母
	return 1;
}

