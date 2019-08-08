/*********************************************************************************
 * 文件名称: jiot_timer.c
 * 作   者:   Ice
 * 版   本:
 **********************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include "jiot_std.h"
#include "jiot_timer.h"

unsigned long GettickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC,&ts); 
    return (ts.tv_sec*1000 + ts.tv_nsec/(1000000));
}


void jiot_timer_assignment(INT32 millisecond,S_JIOT_TIME_TYPE *timer)
{
    timer->time = millisecond;
}

INT32 jiot_timer_now_clock(S_JIOT_TIME_TYPE *timer)
{
    timer->time =GettickCount();

    return JIOT_SUCCESS;
}

INT64 jiot_timer_spend(S_JIOT_TIME_TYPE *start)
{
    UINT64 now = GettickCount();
    INT64 spend = now - start->time;
    return spend;
}

INT64 jiot_timer_remain(S_JIOT_TIME_TYPE *end)
{
    UINT64 now = GettickCount();
    INT64 remain = end->time - now;
    return remain;
}

BOOL jiot_timer_expired(S_JIOT_TIME_TYPE *timer)
{
    UINT64 now = GettickCount();
    
    INT64 expired = timer->time - now;
    
    if(expired <= 0 )
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
    UINT64 now = GettickCount();
    timer->time = now+millisecond;
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

