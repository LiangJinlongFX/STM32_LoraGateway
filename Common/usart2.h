#ifndef __USART2_H
#define __USART2_H
/**
  ******************************************************************************
  * @file    STM32F407DEMO/usart2.h 
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

#include "stdio.h"
#include "stm32f4xx_conf.h"
#include "sys.h"

#define MAX_RCV_LEN  1024
extern volatile unsigned char  gprs_ready_flag;
extern volatile unsigned char  gprs_ready_count;
extern char  usart2_rcv_buf[MAX_RCV_LEN];
extern volatile unsigned int   usart2_rcv_len;

void uart2_init(u32 bound);
void USART2_Clear(void);
void USART2_Write(USART_TypeDef* USARTx, uint8_t *Data, uint8_t len);
void SendCmd(char* cmd, char* result, int timeOut);
uint32_t USART2_GetRcvNum(void);
void  USART2_GetRcvData(uint8_t *buf, uint32_t rcv_len);

#endif


