/*********************************************************************************
 * 文件名称: jiot_timer.h
 * 作   者:   Ice
 * 版   本:
 **********************************************************************************/

#ifndef JIOT_TIMER_H
#define JIOT_TIMER_H

#include <sys/time.h>
#include "jiot_err.h"
#include "jiot_std.h"

#if defined(__cplusplus)
 extern "C" {
#endif

typedef struct JIOT_TIME_TYPE
{
    rt_tick_t time;
}S_JIOT_TIME_TYPE;

/***********************************************************
* 函数名称: jiot_timer_assignment
* 描       述: 时间赋值操作
* 输入参数:      INT32 millisecond          时间，单位毫秒
* 输出参数:      S_JIOT_TIME_TYPE *timer    时间结构体的指针 
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
void jiot_timer_assignment(INT32 millisecond,S_JIOT_TIME_TYPE *timer);

/***********************************************************
* 函数名称: jiot_timer_now_clock
* 描       述:  设置当前时间
* 输入参数:      S_JIOT_TIME_TYPE *timer    时间结构体的指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_timer_now_clock(S_JIOT_TIME_TYPE *timer);

/***********************************************************
* 函数名称: jiot_timer_spend
* 描       述: 计算当前时间和start时间之间的时间差，（now减去start） 单位毫秒
* 输入参数:      S_JIOT_TIME_TYPE *start  时间结构体的指针
* 输出参数:      
* 返 回  值: 当前时间和start时间之间的时间差，单位毫秒
************************************************************/
INT64 jiot_timer_spend(S_JIOT_TIME_TYPE *start);

/***********************************************************
* 函数名称: jiot_timer_remain
* 描       述: 计算end和当前时间之间的时间差，（end减去now）单位毫秒
* 输入参数:      S_JIOT_TIME_TYPE *end  时间结构体的指针
* 输出参数:      
* 返 回  值: end和当前时间之间的时间差，单位毫秒
************************************************************/
INT64 jiot_timer_remain(S_JIOT_TIME_TYPE *end);

/***********************************************************
* 函数名称: jiot_timer_expired
* 描       述: 判断timer的时间点是否已到
* 输入参数:      S_JIOT_TIME_TYPE *timer  时间结构体的指针
* 输出参数:      
* 返 回  值: 时间点已到返回    JIOT_TRUE
            时间点未到返回    JIOT_FALSE
************************************************************/
BOOL jiot_timer_expired(S_JIOT_TIME_TYPE *timer);

/***********************************************************
* 函数名称: jiot_timer_init
* 描       述: 初始化timer为0，不赋值
* 输入参数:      S_JIOT_TIME_TYPE *timer  时间结构体的指针
* 输出参数:      
* 返 回  值: 空
************************************************************/
void jiot_timer_init(S_JIOT_TIME_TYPE* timer);

/***********************************************************
* 函数名称: jiot_timer_countdown
* 描       述: 计算当前时间点向后偏移millisecond毫秒的时间结构
* 输入参数:      S_JIOT_TIME_TYPE *timer  时间结构体的指针
                UINT32      millisecond  时长，单位毫秒
* 输出参数:      S_JIOT_TIME_TYPE *timer  偏移后的时间点的结构体的指针
* 返 回  值: 空
************************************************************/
void jiot_timer_countdown(S_JIOT_TIME_TYPE* timer,UINT32 millisecond);

/***********************************************************
* 函数名称: jiot_timer_now_ms
* 描       述: 计算当前的UTC时间的毫秒数
* 输入参数:      
* 输出参数:      
* 返 回  值: 返回当前的时间，单位毫秒
************************************************************/
UINT64 jiot_timer_now_ms();

/***********************************************************
* 函数名称: jiot_timer_now
* 描       述: 计算当前的UTC时间的秒
* 输入参数:      
* 输出参数:      
* 返 回  值: 返回当前的时间，单位秒
************************************************************/
UINT32 jiot_timer_now();

/***********************************************************
* 函数名称: jiot_timer_s2str
* 描       述: 将时间转换成年月日时分秒的字符串
* 输入参数:      UINT32 second  时间，单位秒
                char* buf 输出buf
* 输出参数:      char* buf 输出根据时间转换成YYYYMMDDHHMISS格式的字符串的buf
* 返 回  值: 
************************************************************/
void jiot_timer_s2str(UINT32 second,char* buf );

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif

