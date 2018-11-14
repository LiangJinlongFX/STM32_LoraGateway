/**
  ******************************************************************************
  * @file    STM32F407DEMO/usart1.c 
  * @author  Liang
  * @version V1.0.0
  * @date    2017-4-26
  * @brief   
  ******************************************************************************
  * @attention
	* 加入是否支持UCOS宏定义
	* 支持适应不同频率下的串口波特率设置.
	*	加入了对printf的支持
	* 增加了串口接收命令功能.
	* 修正了printf第一个字符丢失的bug
	* 修改串口初始化IO的bug
	* 修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
	* 增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
	* 修改了EN_USART1_RX的使能方式
	* 增加了对UCOSII的支持
  ******************************************************************************
**/

/* include-------------------------------------------------- */
#include "sys.h"
#include "usart1.h"
#include "rtthread.h"

//是否使能了系统串口
#ifdef	RT_USING_UART

//是否使能了设备
#ifdef RT_USING_DEVICE
#include <rtdevice.h>
#endif

#define UART_RX_BUFSZ 8   //串口接收缓存大小

/* STM32 uart driver */
struct stm32_uart
{
    struct rt_device parent;
    struct rt_ringbuffer rx_rb;
    USART_TypeDef * uart_base;
    IRQn_Type uart_irq;
    rt_uint8_t rx_buffer[UART_RX_BUFSZ];
    
};

//使用串口1做调试口
#ifdef RT_USING_UART1
struct stm32_uart uart1_device;
#endif

//系统级串口中断函数
void uart_irq_handler(struct stm32_uart* uart)
{
    /* enter interrupt */
    rt_interrupt_enter();
    
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断
    {
				//printf("OK");
        rt_ringbuffer_putchar_force(&(uart->rx_rb), (rt_uint8_t)USART_ReceiveData(uart->uart_base));
        /* invoke callback */
        if(uart->parent.rx_indicate != RT_NULL)
        {
            uart->parent.rx_indicate(&uart->parent, rt_ringbuffer_data_len(&uart->rx_rb));
        }    
    }
    
    /* leave interrupt */
    rt_interrupt_leave();
}

//重映射串口1到rt_kprintf
void rt_hw_console_output(const char *str)
{
   /* empty console output */
	
	rt_enter_critical();

	while(*str!='\0')
	{
		if(*str=='\n')
		{
			USART_SendData(USART1, '\r'); 
			while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
		}
		USART_SendData(USART1, *str++); 
		while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	
	}
	rt_exit_critical();
}

//初始化IO 串口1 
//bound:波特率
void uart1_init(u32 bound){
   //GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟
 
	//串口1对应引脚复用映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1
	
	//USART1端口配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure); //初始化串口1
	
	USART_Cmd(USART1, ENABLE);  //使能串口1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

	
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
		uart_irq_handler(&uart1_device);
}

//系统级串口初始化
static rt_err_t rt_uart_init (rt_device_t dev)
{
//    struct stm32_uart* uart;
//    RT_ASSERT(dev != RT_NULL);
//    uart = (struct stm32_uart *)dev;
    
		uart1_init(115200);

    return RT_EOK;
}
//系统级开启串口
static rt_err_t rt_uart_open(rt_device_t dev, rt_uint16_t oflag)
{
    struct stm32_uart* uart;
    RT_ASSERT(dev != RT_NULL);
    uart = (struct stm32_uart *)dev;

    if (dev->flag & RT_DEVICE_FLAG_INT_RX)
    {
        /* Enable the UART Interrupt */
        NVIC_EnableIRQ(uart->uart_irq);
    }

    return RT_EOK;
}
//系统级关闭串口中断
static rt_err_t rt_uart_close(rt_device_t dev)
{
    struct stm32_uart* uart;
    RT_ASSERT(dev != RT_NULL);
    uart = (struct stm32_uart *)dev;

    if (dev->flag & RT_DEVICE_FLAG_INT_RX)
    {
        /* Disable the UART Interrupt */
        NVIC_DisableIRQ(uart->uart_irq);
    }

    return RT_EOK;
}
//系统级串口读函数
static rt_size_t rt_uart_read(rt_device_t dev, rt_off_t pos, void* buffer, rt_size_t size)
{
    rt_size_t length;
    struct stm32_uart* uart;
    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)dev;
    /* interrupt receive */
    rt_base_t level;

    RT_ASSERT(uart != RT_NULL);

    /* disable interrupt */
    level = rt_hw_interrupt_disable();
    length = rt_ringbuffer_get(&(uart->rx_rb), buffer, size);
    /* enable interrupt */
    rt_hw_interrupt_enable(level);

    return length;
}
//系统级串口写函数
static rt_size_t rt_uart_write(rt_device_t dev, rt_off_t pos, const void* buffer, rt_size_t size)
{
    char *ptr = (char*) buffer;
    struct stm32_uart* uart;
    RT_ASSERT(serial != RT_NULL);
    uart = (struct stm32_uart *)dev;

    if (dev->open_flag & RT_DEVICE_FLAG_STREAM)
    {
        /* stream mode */
        while (size)
        {
            if (*ptr == '\n')
            {
                while(RESET == USART_GetFlagStatus(uart->uart_base,USART_FLAG_TXE));
                USART_SendData(uart->uart_base, '\r');   
            }

            while(RESET == USART_GetFlagStatus(uart->uart_base,USART_FLAG_TXE));
            USART_SendData(uart->uart_base, *ptr); 

            ptr ++;
            size --;
        }
    }
    else
    {
        while (size)
        {
            while(RESET == USART_GetFlagStatus(uart->uart_base,USART_FLAG_TXE));
            USART_SendData(uart->uart_base, *ptr); 

            ptr++;
            size--;
        }
    }

    return (rt_size_t) ptr - (rt_size_t) buffer;
}

/*
	系统级串口初始化函数
	使用串口1
*/
int rt_hw_usart_init(void)
{
		struct stm32_uart* uart;

		/* get uart device */
		uart = &uart1_device;

		/* device initialization */
		uart->parent.type = RT_Device_Class_Char;
		uart->uart_base = USART1;
		uart->uart_irq = USART1_IRQn;
		rt_ringbuffer_init(&(uart->rx_rb), uart->rx_buffer, sizeof(uart->rx_buffer));

		/* device interface */
		uart->parent.init 	    = rt_uart_init;
		uart->parent.open 	    = rt_uart_open;
		uart->parent.close      = rt_uart_close;
		uart->parent.read 	    = rt_uart_read;
		uart->parent.write      = rt_uart_write;
		uart->parent.control    = RT_NULL;
		uart->parent.user_data  = RT_NULL;

		rt_device_register(&uart->parent, "uart1", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    return 0;	
}
INIT_BOARD_EXPORT(rt_hw_usart_init);

#endif	

