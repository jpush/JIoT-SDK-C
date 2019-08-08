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
#include "m5311_opencpu.h"



static UINT32 g_time_offset = 0;
static INT32  g_timezone = 0;
void getTimeOffset()
{
    unsigned char time_string[64]={0};
    opencpu_rtc_get_time(time_string);
    if(time_string[0] == 0)
    {
        return ;
    }

    time_t t;
    struct tm stm;
    memset(&stm, 0, sizeof(struct tm));
    char *p = strptime(time_string,"%Y/%m/%d,%H:%M:%S", &stm);
    if(NULL != p && 0 == strncmp(p, "GMT", 3))
    {
        g_timezone = (('+' == p[3])?atoi(p+4):(0-atoi(p+4)));
        t =  mktime(&stm) - (g_timezone * 60 * 60); // tz="GMT+0"
    }

    //计算偏移量
    g_time_offset =t - time(NULL);

    return;
}


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

    if (g_time_offset == 0)
    {
        getTimeOffset();
    }
    return now.tv_sec + g_time_offset;
}

void jiot_timer_s2str(UINT32 second,char* buf )
{
    struct tm stm;
    time_t timer = second + g_timezone*3600;
    gmtime_r(&timer, &stm);

    sprintf(buf,"%d%02d%02d%02d%02d%02d", stm.tm_year+1900,stm.tm_mon+1,stm.tm_mday,stm.tm_hour,stm.tm_min,stm.tm_sec);

}

