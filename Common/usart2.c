/**
  ******************************************************************************
  * @file    STM32F407DEMO/usart2.c 
  * @author  Liang
  * @version V1.0.0
  * @date    2018-1-22
  * @brief   
  ******************************************************************************
  * @attention
	* 加入对rt-thread的支持
	*	用于rt-thread msh的串口驱动
  ******************************************************************************
**/


/* include-------------------------------------------------- */
#include "sys.h"
#include "usart2.h"
#include "delay.h"
#include "rtthread.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
//#include "utils.h"

volatile unsigned char  gprs_ready_flag = 0;
volatile unsigned char  gprs_ready_count = 0;

char  usart2_rcv_buf[MAX_RCV_LEN];
volatile unsigned int   usart2_rcv_len = 0;

//硬件级串口初始化
void uart2_init(u32 bound)
{
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);//使能USART2时钟
 
	//串口2对应引脚复用映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource2,GPIO_AF_USART2); //GPIOA2复用为USART2
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_USART2); //GPIOA3复用为USART2
	
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA2，PA3

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART2, &USART_InitStructure); //初始化串口2
	
	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =4;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器

	USART_Cmd(USART2, ENABLE);  //使能串口2
	
	USART_ClearFlag(USART2, USART_FLAG_TC);
	
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启相关中断
	
}

/**
 * 清空串口2接收缓存
 * @param   
 * @return 
 * @brief 
 **/
void USART2_Clear(void)
{
	memset(usart2_rcv_buf, 0, strlen(usart2_rcv_buf));
	usart2_rcv_len = 0;
}

/**
 * 串口发送
 * @param   
 * @return 
 * @brief 
 **/
void USART2_Write(USART_TypeDef* USARTx, uint8_t *Data, uint8_t len)
{
    uint8_t i;
    USART_ClearFlag(USARTx, USART_FLAG_TC);
    for(i = 0; i < len; i++)
    {
        USART_SendData(USARTx, *Data++);
        while( USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET );
    }
}

/**
 * 串口发送AT指令
 * @param   
 * @return 
 * @brief 
 **/
void SendCmd(char* cmd, char* result, int timeOut)
{
    while(1)
    {
        USART2_Clear();
        USART2_Write(USART2, cmd, strlen(cmd));
        delay_ms(timeOut);
        if((NULL != strstr(usart2_rcv_buf, result)))	//判断是否有预期的结果
        {
            break;
        }
        else
        {
            delay_ms(100);
        }
    }
}

/**
 * 返回串口接受到的数据长度
 * @param   
 * @return 
 * @brief 
 **/
uint32_t USART2_GetRcvNum(void)
{
	static uint32_t len = 0;
	uint32_t result = 0;
	
	if(usart2_rcv_len == 0)
	{
		len = 0;
		result = 0;
	}
	else if(len != usart2_rcv_len)
	{
		result = usart2_rcv_len - len;	//新接收长度
		len = usart2_rcv_len;			//保存新长度
	}
	
	return result;
}

/**
 * 复制串口接收到的数据到buf
 * @param   
 * @return 
 * @brief 
 **/
void  USART2_GetRcvData(uint8_t *buf, uint32_t rcv_len)
{
    if(buf)
    {
        memcpy(buf, usart2_rcv_buf, rcv_len);
    }
    //USART2_Clear();
}
