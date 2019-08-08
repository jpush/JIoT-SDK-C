/*********************************************************************************
 * 文件名称: jiot_timer.c
 * 作   者:   Ice
 * 版   本:
 **********************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include "jiot_std.h"
#include "jiot_timer.h"

void jiot_timer_assignment(INT32 millisecond,S_JIOT_TIME_TYPE *timer)
{
    timer->time = (struct timeval){millisecond / 1000, (millisecond % 1000) * 1000};
}

INT32 jiot_timer_now_clock(S_JIOT_TIME_TYPE *timer)
{
    gettimeofday(&timer->time, NULL);

    return JIOT_SUCCESS;
}

INT64 jiot_timer_spend(S_JIOT_TIME_TYPE *start)
{
    struct timeval now, res;

    gettimeofday(&now, NULL);
    timersub(&now, &start->time, &res);
    return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}

INT64 jiot_timer_remain(S_JIOT_TIME_TYPE *end)
{
    struct timeval now, res;

    gettimeofday(&now, NULL);
    timersub(&end->time, &now, &res);
    return (res.tv_sec)*1000 + (res.tv_usec)/1000;
}

BOOL jiot_timer_expired(S_JIOT_TIME_TYPE *timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->time, &now, &res);
    if(res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0))
    {
        return JIOT_TRUE;
    }

    return JIOT_FALSE;
}

void jiot_timer_init(S_JIOT_TIME_TYPE* timer)
{
    timer->time = (struct timeval){0, 0};
}

void jiot_timer_countdown(S_JIOT_TIME_TYPE* timer,UINT32 millisecond)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {millisecond / 1000, (millisecond % 1000) * 1000};
    timeradd(&now, &interval, &timer->time);
}

UINT64 jiot_timer_now_ms()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec*1000 + now.tv_usec/1000 ;
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

