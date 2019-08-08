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
#include "task.h"


TickType_t tick_from_millisecond(INT32 millisecond)
{
    return millisecond/portTICK_RATE_MS;
}

INT64 tick_to_millisecond(TickType_t tick)
{
    return tick*portTICK_RATE_MS;
}

void jiot_timer_assignment(INT32 millisecond,S_JIOT_TIME_TYPE *timer)
{
    timer->time = tick_from_millisecond(millisecond);
}

INT32 jiot_timer_now_clock(S_JIOT_TIME_TYPE *timer)
{
    timer->time = xTaskGetTickCount();
    return (INT32)JIOT_SUCCESS;
}

INT64 jiot_timer_spend(S_JIOT_TIME_TYPE *start)
{
    TickType_t now = xTaskGetTickCount();
    TickType_t spendTick = now - start->time;
    return tick_to_millisecond(spendTick);
}

INT64 jiot_timer_remain(S_JIOT_TIME_TYPE *end)
{
    TickType_t now = xTaskGetTickCount();
    
    TickType_t remainTick = end->time - now;
    return tick_to_millisecond(remainTick);
}

BOOL jiot_timer_expired(S_JIOT_TIME_TYPE *timer)
{
    TickType_t now = xTaskGetTickCount();
    
    TickType_t expiredTick = timer->time - now;
    
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
    TickType_t now = xTaskGetTickCount();
    
    timer->time = now+tick_from_millisecond(millisecond);
}

UINT32 jiot_timer_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;
}

void jiot_timer_s2str(UINT32 second,char* buf )
{
    struct tm *tblock;
    time_t timer = second;
    tblock = localtime(&timer);

    sprintf(buf,"%d%02d%02d%02d%02d%02d", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);
    return  ;
}


