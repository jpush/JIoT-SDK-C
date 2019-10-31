/*********************************************************************************
 * 文件名称: jiot_pthread.c
 * 作   者: Ice
 * 版   本:
 **********************************************************************************/

#include "jiot_err.h"
#include "jiot_pthread.h"


INT32 jiot_mutex_init(S_JIOT_MUTEX *mutex)
{
    INT32 nRet = pthread_mutex_init(&(mutex->lock),NULL);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_lock(S_JIOT_MUTEX *mutex)
{
    INT32 nRet = pthread_mutex_lock(&mutex->lock);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_unlock(S_JIOT_MUTEX *mutex)
{
    INT32 nRet = pthread_mutex_unlock(&mutex->lock);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_destory(S_JIOT_MUTEX *mutex)
{
    INT32 nRet = pthread_mutex_destroy(&mutex->lock);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_create(S_JIOT_PTHREAD* handle,void*(*func)(void*),void *arg, const char* name, INT32 size)
{
    INT32 nRet = JIOT_SUCCESS;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    nRet = pthread_create(&handle->threadID,&attr,func,arg);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_exit(S_JIOT_PTHREAD* handle)
{
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_cancel(S_JIOT_PTHREAD*handle)
{
    INT32 nRet = JIOT_SUCCESS;
    if(handle->threadID != 0)
    {
    	pthread_cancel(handle->threadID);
//        nRet = pthread_cancel(handle->threadID); //这里会cancel一个已经执行完的线程而报错ESRCH
//        if (nRet != 0)
//        {
//            return JIOT_FAIL;
//        }
    }

    return JIOT_SUCCESS;
}

INT32 jiot_sleep(INT32 millisecond)
{
    INT32 nRet = usleep(millisecond*1000);
    if (nRet != 0)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

