/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/
#include "diskio.h"					/* FatFs lower layer API */
#include "sdio_sdcard.h"		/*包含SD卡头文件*/
#include "malloc.h"					/*包含内存管理头文件*/
/**
  ******************************************************************************
  * @attention
	* FATFS底层驱动函数
	* 只包含SD卡这一个驱动器
	* 包含内存管理
	* 包含RTC时钟
  ******************************************************************************
**/

/* Private define ------------------------------------------------------------*/
#define SD_CARD	 0  	//SD卡,卷标为0

#define FLASH_SECTOR_SIZE 	512			  //扇区大小    512K

#define FLASH_BLOCK_SIZE   	8     		//每个BLOCK有8个扇区


/********************************
//初始化磁盘
参数：设备卷标 默认SD卡卷标为0；
返回值：初始化结果 1初始化失败或SD卡没插入 0：初始化成功
*********************************/
DSTATUS disk_initialize(BYTE pdrv)	/* Physical drive nmuber (0..) */
{
	u8 res=0;	    
	switch(pdrv)
	{
		case SD_CARD:				//SD卡
			res=SD_Init();		//SD卡初始化 
  	break;
		default:
			res=1; 
	}		 
	if(res)return  STA_NOINIT;
	else return 0; //初始化成功
}  

//获得磁盘状态/* Physical drive nmuber (0..) */
DSTATUS disk_status (BYTE pdrv)
{ 
	return 0;
} 

/************************************
函数名：读扇区
参数：
drv:磁盘编号0~9	Physical drive nmuber (0..)
*buff:数据接收缓冲首地址	Data buffer to store read data
sector:扇区地址	Sector address (LBA)
count:需要读取的扇区数	Number of sectors to read (1..128)
*************************************/
DRESULT disk_read (BYTE pdrv,	BYTE *buff,DWORD sector,UINT count)
{
	u8 res=0; 
  if (!count)	return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case SD_CARD://SD卡
			res=SD_ReadDisk(buff,sector,count);	 
			while(res)//读出错
			{
				SD_Init();	//重新初始化SD卡
				res=SD_ReadDisk(buff,sector,count);	
				//printf("sd rd error:%d\r\n",res);
			}
			break;
		default:
			res=1; 
	}
  //处理返回值，将SPI_SD_driver.c的返回值转成ff.c的返回值
  if(res==0x00)return RES_OK;	 
  else return RES_ERROR;	   
}

/********************************************
函数名：写扇区
参数：
drv:磁盘编号0~9	Physical drive nmuber (0..)
*buff:发送数据首地址	Data to be written
sector:扇区地址	Sector address (LBA)
count:需要写入的扇区数	Number of sectors to write (1..128)
*********************************************/
#if _USE_WRITE
DRESULT disk_write (BYTE pdrv,const BYTE *buff,DWORD sector,UINT count)
{
	u8 res=0;  
    if (!count)return RES_PARERR;//count不能等于0，否则返回参数错误		 	 
	switch(pdrv)
	{
		case SD_CARD://SD卡
			res=SD_WriteDisk((u8*)buff,sector,count);
			while(res)//写出错
			{
				SD_Init();	//重新初始化SD卡
				res=SD_WriteDisk((u8*)buff,sector,count);	
				//printf("sd wr error:%d\r\n",res);
			}
			break;
		default:
			res=1; 
	}
    //处理返回值，将SPI_SD_driver.c的返回值转成ff.c的返回值
    if(res == 0x00)return RES_OK;	 
    else return RES_ERROR;	
}
#endif

/********************************************
函数名：其他表参数的获得
参数：
drv:磁盘编号0~9	Physical drive nmuber (0..)
ctrl:控制代码	Control code
*buff:发送/接收缓冲区指针	Buffer to send/receive control data
*********************************************/
#if _USE_IOCTL
DRESULT disk_ioctl (BYTE pdrv,BYTE cmd,void *buff	)
{
	DRESULT res;						  			     
	if(pdrv==SD_CARD)//SD卡
	{
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				res = RES_OK; 
		        break;	 
		    case GET_SECTOR_SIZE:
				*(DWORD*)buff = 512; 
		        res = RES_OK;
		        break;	 
		    case GET_BLOCK_SIZE:
				*(WORD*)buff = SDCardInfo.CardBlockSize;
		        res = RES_OK;
		        break;	 
		    case GET_SECTOR_COUNT:
		        *(DWORD*)buff = SDCardInfo.CardCapacity/512;
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}
	else res=RES_ERROR;//其他的不支持
    return res;
}
#endif
//获得时间
//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{				 
	return 0;
}			 
//动态分配内存
void *ff_memalloc (UINT size)			
{
	return (void*)mymalloc(SRAMIN,size);
}
//释放内存
void ff_memfree (void* mf)		 
{
	myfree(SRAMIN,mf);
}

















