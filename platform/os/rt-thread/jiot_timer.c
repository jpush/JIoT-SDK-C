/*********************************************************************************
 * 文件名称: jiot_timer.c
 * 作   者:   Ice
 * 版   本:
 **********************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include "jiot_std.h"
#include "jiot_timer.h"



#define DECSEPER 	1000000000ULL			// 10^9
#define DECSEPER2 	1000000000000000000ULL	// 10^18

void jiot_timer_assignment(INT32 millisecond,S_JIOT_TIME_TYPE *timer)
{
    timer->time = rt_tick_from_millisecond(millisecond);
}

INT32 jiot_timer_now_clock(S_JIOT_TIME_TYPE *timer)
{
    timer->time = rt_tick_get();

    return (INT32)JIOT_SUCCESS;
}

INT64 jiot_timer_spend(S_JIOT_TIME_TYPE *start)
{
    rt_tick_t now = rt_tick_get();
    rt_tick_t spendTick = now - start->time;
    return (spendTick*1000)/RT_TICK_PER_SECOND ;
}

INT64 jiot_timer_remain(S_JIOT_TIME_TYPE *end)
{
    rt_tick_t now = rt_tick_get();
    
    rt_tick_t remainTick = end->time - now;
    return (remainTick*1000)/RT_TICK_PER_SECOND ;;
}

BOOL jiot_timer_expired(S_JIOT_TIME_TYPE *timer)
{
    rt_tick_t now = rt_tick_get();
    
    rt_tick_t expiredTick = timer->time - now;
    
    if(expiredTick <= 0 )
    {
        return JIOT_TRUE;
    }
    
    return JIOT_FALSE ;
}

void jiot_timer_init(S_JIOT_TIME_TYPE* timer)
{
    timer->time = 0;
}

void jiot_timer_countdown(S_JIOT_TIME_TYPE* timer,UINT32 millisecond)
{
    rt_tick_t now = rt_tick_get();
    
    timer->time = now+rt_tick_from_millisecond(millisecond);
}

UINT64 jiot_timer_now_ms()
{
	return time(NULL) * 1000;
    /*struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec*1000 + now.tv_usec/1000 ;*/
	/*由于有些环境无法使用gettimeofday函数，因此使用系统tick来实现该功能*/
	/*
#if (RT_TICK_PER_SECOND == 1000)
    return (UINT64)rt_tick_get();
#else

    uint64_t tick;
    tick = rt_tick_get();

    tick = tick * 1000;

    return (tick + RT_TICK_PER_SECOND - 1)/RT_TICK_PER_SECOND;
#endif
*/
}

UINT32 jiot_timer_now()
{
	return time(NULL);
    /*struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;*/
	/*由于有些环境无法使用gettimeofday函数，因此使用系统tick来实现该功能*/
	/*
#if (RT_TICK_PER_SECOND == 1000)
    return (UINT32)(rt_tick_get() / 1000);
#else

    uint64_t tick;
    tick = rt_tick_get();

    tick = tick * 1000;

    return (tick + RT_TICK_PER_SECOND - 1)/RT_TICK_PER_SECOND / 1000;
#endif
*/
}

void jiot_timer_s2str(UINT32 second,char* buf )
{
    struct tm *tblock;
    time_t timer = second;
    tblock = localtime(&timer);

    sprintf(buf,"%d%02d%02d%02d%02d%02d", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);
    return  ;
}

