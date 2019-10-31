#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "jiot_err.h"
#include "jiot_file.h"


INT32 jiot_file_fopen(S_JIOT_FILE_HANDLE *handle,char* filename,E_JIOT_FILE_FLAG flags)
{
    if(handle == NULL || filename == NULL)
    {
        return JIOT_FAIL;
    }

    char* flagslinux = NULL;

    switch (flags)
    {
        case RD_FLAG:
            flagslinux = "r";
            break;
        case RDWR_FALG:
            flagslinux = "r+";
            break;
        case CWR_FALG:
            flagslinux = "w";
            break;
        case CRDWR_FLAG:
            flagslinux = "w+";
            break;
        case CA_FLAG:
            flagslinux = "a";
            break;
        case CARDWR_FLAG:
            flagslinux = "a+";
            break;
        default:
            return JIOT_FAIL;
            break;
    }

    handle->fd = fopen(filename,flagslinux);
    if (handle->fd == NULL)
    {
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_file_fclose(S_JIOT_FILE_HANDLE*handle)
{
    return fclose(handle->fd);
}

INT32 jiot_file_fwrite(S_JIOT_FILE_HANDLE* handle,char* buf,INT32 size,INT32 count)
{
    INT32 lens = fwrite(buf, size,count,handle->fd);
    if (lens != count )
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_file_fread(S_JIOT_FILE_HANDLE* handle,char* buf,INT32 size,INT32 count)
{
    INT32 lens = fread(buf, size,count,handle->fd);

    return lens;
}

INT32 jiot_file_fseek(S_JIOT_FILE_HANDLE* handle,INT32 offset,E_JIOT_FILE_OFFSE_FLAG flags)
{

    int origin = 0;
    switch (flags)
    {
        case BEGIN_FLAG:
            origin = SEEK_SET;
            break;
        case END_FALG:
            origin = SEEK_END;
            break;
        default:
            return JIOT_FAIL;
            break;
    }

    if(0 == fseek(handle->fd,offset,origin))
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_file_ftell(S_JIOT_FILE_HANDLE* handle)
{
    INT32 result = 0;
    result = ftell(handle->fd);

    if (-1 == result)
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_file_access(char* filename)
{
    if (0 != access(filename, 0))
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

INT32 jiot_file_remove(char* filename)
{
    if(0 != remove(filename))
    {
        return JIOT_FAIL;
    }
    return JIOT_SUCCESS;
}

