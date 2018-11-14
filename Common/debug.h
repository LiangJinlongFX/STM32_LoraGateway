#ifndef _DEBUG_H
#define _DEBUG_H
/**
  * @file    debug.h
  * @author  Liang
  * @version V1.0.0
  * @date    2018-9-10
  * @brief	 用于系统调试日志输出控制
  **/
#include <rtthread.h>

#define __DEBUG 1

#ifdef __DEBUG
#define logging_debug(...) do{/*rt_kprintf("[FILE: %s, LINE: %d ] DEBUG: ", __FILE__,__LINE__);*/rt_kprintf(__VA_ARGS__);rt_kprintf("\r\n");}while(0)
#define logging_warning(...) do{rt_kprintf("[FILE: %s, LINE: %d ] WARNING: ", __FILE__,__LINE__);rt_kprintf(__VA_ARGS__);rt_kprintf("\r\n");}while(0)
#define logging_info(...) do{rt_kprintf("[FILE: %s, LINE: %d ] INFO: ", __FILE__,__LINE__);rt_kprintf(__VA_ARGS__);rt_kprintf("\r\n");}while(0)
#define logging_error(...) do{rt_kprintf("[FILE: %s, LINE: %d ]: ERROR:	", __FILE__,__LINE__);rt_kprintf(__VA_ARGS__);rt_kprintf("\r\n");}while(0)
#else
#define logging_debug(info)
#define logging_warning(info)
#define logging_info(info)
#define logging_error(info)
#endif

#endif