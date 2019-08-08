#include "jiot_err.h"
#include "jiot_std.h"
#include <errno.h>


INT32 jiot_get_errno(void)
{
    INT32 jiot_errno = 0 ;
    switch(errno)
    {
        case EINTR:
            jiot_errno = JIOT_ERR_NET_EINTR ;
            break;
        case EAGAIN:       
            jiot_errno = JIOT_ERR_NET_EAGAIN ;
            break;
        /*
        case EWOULDBLOCK:       
            jiot_errno = JIOT_ERR_NET_EWOULDBLOCK ;
             break;
        */
        default:
            jiot_errno = 0-(JIOT_SUCCESS+errno);
            break;
    }
    
    return jiot_errno;
}