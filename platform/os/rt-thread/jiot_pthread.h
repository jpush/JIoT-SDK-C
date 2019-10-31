#ifndef JIOT_PTHREAD_H
#define JIOT_PTHREAD_H
#include <rtthread.h>

#include "jiot_std.h"

#if defined(__cplusplus)
 extern "C" {
#endif


typedef struct JIOT_MUTEX
{
	char mutex_name[16];
	rt_mutex_t  lock;
}S_JIOT_MUTEX;

typedef struct JIOT_PTHREAD
{
	rt_thread_t threadID;
    int status ;
}S_JIOT_PTHREAD;


/***********************************************************
* 函数名称: jiot_mutex_init
* 描       述: 初始化互斥锁
* 输入参数:      S_JIOT_MUTEX* mutex   互斥锁指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_mutex_init(S_JIOT_MUTEX* mutex);

/***********************************************************
* 函数名称: jiot_mutex_lock
* 描       述: 加锁
* 输入参数:      S_JIOT_MUTEX* mutex   互斥锁指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_mutex_lock(S_JIOT_MUTEX* mutex);

/***********************************************************
* 函数名称: jiot_mutex_unlock
* 描       述: 解锁
* 输入参数:      S_JIOT_MUTEX* mutex   互斥锁指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_mutex_unlock(S_JIOT_MUTEX* mutex);

/***********************************************************
* 函数名称: jiot_mutex_destory
* 描       述: 销毁互斥锁
* 输入参数:      S_JIOT_MUTEX* mutex   互斥锁指针
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_mutex_destory(S_JIOT_MUTEX* mutex);

/***********************************************************
* 函数名称: jiot_pthread_create
* 描       述: 创建任务线程
* 输入参数:      S_JIOT_PTHREAD* handle   线程句柄指针  
                void*(*func)(void*)      线程入口函数
                void *arg                线程参数
                const char* name,        线程名
                INT32 size                 线程堆栈大小
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_pthread_create(S_JIOT_PTHREAD* handle,void(*func)(void*),void *arg, const char* name, INT32 size);

/***********************************************************
* 函数名称: jiot_pthread_exit
* 描       述: 线程内部退出线程的接口函数，对于部分嵌入式操作系统使用，如freeRTOS，如不支持可以在接口内部直接返回JIOT_SUCCESS
* 输入参数:      S_JIOT_PTHREAD* handle   线程句柄指针 
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_pthread_exit(S_JIOT_PTHREAD* handle);

/***********************************************************
* 函数名称: jiot_pthread_cancel
* 描       述: 外部主动调用线程退出函数接口，如不支持可以在接口内部直接返回JIOT_SUCCESS
* 输入参数:      S_JIOT_PTHREAD* handle   线程句柄指针 
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_pthread_cancel(S_JIOT_PTHREAD* handle);

/***********************************************************
* 函数名称: jiot_sleep
* 描       述: 线程休眠
* 输入参数:      INT32 millisecond  时间，单位毫秒
* 输出参数:      
* 返 回  值: 成功返回    JIOT_SUCCESS
            失败返回    JIOT_FAIL
************************************************************/
INT32 jiot_sleep( INT32 millisecond);


#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
}
#endif

#endif



