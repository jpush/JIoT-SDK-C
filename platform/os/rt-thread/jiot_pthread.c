/*********************************************************************************
 * 文件名称: jiot_pthread.c
 * 作   者: Ice
 * 版   本:
 **********************************************************************************/

#include "jiot_err.h"
#include "jiot_pthread.h"
#define THREAD_PRIORITY         11
#define THREAD_TIMESLICE        10

INT32 jiot_mutex_init(S_JIOT_MUTEX *mutex)
{
	mutex->lock = rt_mutex_create(mutex->mutex_name,RT_IPC_FLAG_FIFO);
	if (mutex->lock == RT_NULL)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_lock(S_JIOT_MUTEX *mutex)
{
    rt_err_t  nRet = rt_mutex_take(mutex->lock,RT_WAITING_FOREVER);
    if (nRet != RT_EOK)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;

}

INT32 jiot_mutex_unlock(S_JIOT_MUTEX *mutex)
{
    rt_err_t nRet = rt_mutex_release(mutex->lock);
    if (nRet != RT_EOK)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;

}

INT32 jiot_mutex_destory(S_JIOT_MUTEX *mutex)
{
    rt_err_t  nRet = rt_mutex_delete(mutex->lock);
    if (nRet != RT_EOK)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;

}

INT32 jiot_pthread_create(S_JIOT_PTHREAD* handle,void(*func)(void*),void *arg, const char* name, INT32 size)
{
	INT32 nRet = JIOT_SUCCESS;
	
	handle->threadID = rt_thread_create(name,func,arg,size,THREAD_PRIORITY,THREAD_TIMESLICE);
	
	
	if(handle->threadID != RT_NULL)
	{
		rt_thread_startup(handle->threadID);
	}
	else
	{
		return JIOT_FAIL;
	}
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_exit(S_JIOT_PTHREAD* handle)
{
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_cancel(S_JIOT_PTHREAD* handle)
{
	
	/*
	rt_err_t nRet = rt_thread_delete(handle->threadID);
    
    if (nRet != RT_EOK)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
	*/
	
	
	int count = 0 ;
    while(1)
    {
        if (handle->status == 0)
        {
            break;
        }
        count++;
        if (count >1000)
        {
            break;
        }

        jiot_sleep(10);
    }
    
    return JIOT_SUCCESS;

}

INT32 jiot_sleep(INT32 millisecond)
{
	rt_err_t nRet = rt_thread_mdelay(millisecond);
    if (nRet != RT_EOK)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

