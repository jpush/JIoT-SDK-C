/*********************************************************************************
 * 文件名称: jiot_pthread.c
 * 作   者: Ice
 * 版   本:
 **********************************************************************************/

#include "jiot_err.h"
#include "jiot_pthread.h"

INT32 jiot_mutex_init(S_JIOT_MUTEX *mutex)
{
	if( mutex == NULL )
	{
        return JIOT_FAIL;
	}
    mutex->lock = xSemaphoreCreateMutex();
    mutex->is_valid = mutex->lock != NULL;
	return JIOT_SUCCESS;
}

INT32 jiot_mutex_lock(S_JIOT_MUTEX *mutex)
{
    if( mutex == NULL || ! mutex->is_valid )
    {
        return JIOT_FAIL;
    }

    if( xSemaphoreTake( mutex->lock, portMAX_DELAY ) != pdTRUE )
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_unlock(S_JIOT_MUTEX *mutex)
{
    if( mutex == NULL || ! mutex->is_valid )
    {
        return JIOT_FAIL;
    }

    if( xSemaphoreGive( mutex->lock ) != pdTRUE )
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_mutex_destory(S_JIOT_MUTEX *mutex)
{
    if( mutex == NULL || ! mutex->is_valid )
    {
    	return JIOT_FAIL;
    }

    (void) vSemaphoreDelete(mutex->lock);

    return JIOT_SUCCESS;
}

INT32 jiot_pthread_create(S_JIOT_PTHREAD* handle,void*(*func)(void*),void *arg, const char* name, INT32 size)
{
	if(pdTRUE != xTaskCreate((void(*)(void*))func, name, size, arg, 1, &handle->threadID))
	{
        return JIOT_FAIL;
	}
    
    handle->status = 1;
    return JIOT_SUCCESS;
}

INT32 jiot_pthread_exit(S_JIOT_PTHREAD* handle)
{
    handle->status = 0;
    vTaskDelete(NULL);

    return JIOT_SUCCESS;
}

INT32 jiot_pthread_cancel(S_JIOT_PTHREAD* handle)
{
    int count = 0 ;
    while(true)
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
	vTaskDelay(millisecond/portTICK_RATE_MS);
    return JIOT_SUCCESS;
}

