#ifndef __DELAY_H
#define __DELAY_H 			   
  /**
  ******************************************************************************
  * @file    STM32F407DEMO/delay.c 
  * @author  Liang
  * @version V1.0.0
  * @date    2017-4-26
  * @brief   
  ******************************************************************************
  * @attention
	* 加入对rt_thread的支持
  ******************************************************************************
**/

#include <sys.h>	

void delay_init(u8 sysclk);
void delay_ms(u16 nms);
void delay_us(u32 nus);

#endif





























