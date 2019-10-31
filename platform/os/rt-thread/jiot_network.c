

#include "jiot_network.h"


INT32 jiot_network_send(INT32 sockFd, void *buf, INT32 nbytes)
{
    INT32 nRet = JIOT_FAIL;
    INT32 flag = 0;

    if( sockFd < 0 )
    {
        return JIOT_FAIL;
    }

    nRet = send(sockFd,buf,nbytes,flag);

    if (nRet < 0)
    {
        INT32 ErrCode = jiot_get_errno();
        if(ErrCode == JIOT_ERR_NET_EINTR || ErrCode == JIOT_ERR_NET_EAGAIN || ErrCode == JIOT_ERR_NET_EWOULDBLOCK)
        {
            return  JIOT_ERR_TCP_RETRY;
        }
        else
        {
            return JIOT_FAIL;    
        }
    }
    return nRet;
}

INT32 jiot_network_recv(INT32 sockFd, void *buf, INT32 nbytes, E_TRANS_FLAGS flags)
{
    INT32 nRet = JIOT_FAIL;
    INT32 flag = 0;

    if( sockFd < 0 )
    {
        return JIOT_FAIL;
    }

    if(TRANS_FLAGS_DEFAULT == flags)
    {
        flag = 0;
    }
    else
    {
        flag = MSG_DONTWAIT;
    }

    nRet = recv(sockFd,buf,nbytes,flag);

    if (nRet < 0)
    {
        INT32 ErrCode = jiot_get_errno();
        if(ErrCode == JIOT_ERR_NET_EINTR || ErrCode == JIOT_ERR_NET_EAGAIN || ErrCode == JIOT_ERR_NET_EWOULDBLOCK)
        {
            return  JIOT_ERR_TCP_RETRY;
        }
        else
        {
            return JIOT_FAIL;    
        }
    }
    return nRet;
}

INT32 jiot_network_select(INT32 fd,E_TRANS_TYPE type,INT32 timeoutMs)
{
    struct timeval *timePointer = NULL;
    fd_set *rd_set = NULL;
    fd_set *wr_set = NULL;
    fd_set *ep_set = NULL;
    int rc = 0;
    fd_set sets;
    
    if( fd < 0 )
    {
        return JIOT_FAIL;
    }

    FD_ZERO(&sets);
    FD_SET(fd, &sets);

    if(TRANS_RECV == type)
    {
        rd_set = &sets;
    }
    else
    {
        wr_set = &sets;
    }

    struct timeval timeout = {timeoutMs/1000, (timeoutMs%1000)*1000};
    if(0 != timeoutMs)
    {
        timePointer = &timeout;
    }
    else
    {
        timePointer = NULL;
    }

    rc = select(fd+1,rd_set,wr_set,ep_set,timePointer);
    if (0 == rc)
    {
        return JIOT_ERR_TCP_SELECT_TIMEOUT ;
    }
    else
    if (rc < 0)
    {
        INT32 ErrCode = jiot_get_errno();
        if(ErrCode == JIOT_ERR_NET_EINTR)
        {
            return  JIOT_ERR_TCP_RETRY ;
        }
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_network_settimeout(INT32 fd,E_TRANS_TYPE type,INT32 timeoutMs)
{
    int opt  = 0;
    if( fd < 0 )
    {
        return JIOT_FAIL;
    }

    struct timeval timeout = {timeoutMs/1000, (timeoutMs%1000)*1000};

    if (type == TRANS_RECV)
    {
        opt = SO_RCVTIMEO;
    } 
    else
    {
        opt = SO_SNDTIMEO;
    } 

    if(0 != setsockopt(fd, SOL_SOCKET, opt, (char *)&timeout, sizeof(timeout)))
    {
        return JIOT_ERR_TCP_SETOPT_TIMEOUT;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_network_set_nonblock(INT32 fd)
{
    if( fd < 0 )
    {
        return JIOT_FAIL;
    }

    INT32 flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, (flags | O_NONBLOCK)) < 0)
    {
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_network_set_block(INT32 fd)
{
    if( fd < 0 )
    {
        return JIOT_FAIL;
    }

    INT32 flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, (flags & (~O_NONBLOCK))) < 0)
    {
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_network_close(INT32 fd)
{
    if ( close(fd) < 0)
    {
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}

INT32 jiot_network_shutdown(INT32 fd,INT32 how)
{
    if ( shutdown(fd,how) < 0)
    {
        return JIOT_FAIL;
    }

    return JIOT_SUCCESS;
}


INT32 jiot_network_create(int * pfd,const char *host,const char* port, E_NET_TYPE type)
{
    INT32 nRet = JIOT_SUCCESS;

    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    INT32 fd = 0;

    memset( &hints, 0, sizeof(hints));

    if(NET_IP4 == type)
    {
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
    }
    else
    if (NET_IP6 == type)
    {
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
    }
    else
    {
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;        
    }

    if (getaddrinfo( host, port, &hints, &addrInfoList )!= 0 )
    {
        freeaddrinfo(addrInfoList);
        return JIOT_ERR_TCP_UNKNOWN_HOST;
    }

    for( cur = addrInfoList; cur != NULL; cur = cur->ai_next )
    {
        if (!((cur->ai_family != AF_INET)||(cur->ai_family != AF_INET6)))
        {
            nRet = JIOT_ERR_TCP_HOST_NET_TYPE ;
            continue;
        }
        
        fd = (int) socket( cur->ai_family, cur->ai_socktype,cur->ai_protocol );
        if( fd < 0 )
        {
            nRet = JIOT_ERR_TCP_SOCKET_CREATE ;

            close( fd );
            fd = -1 ;
            continue;
        }

        nRet =jiot_network_settimeout(fd,TRANS_SEND,10000);
        if(nRet == JIOT_SUCCESS )
        {
            if( connect( fd,cur->ai_addr,cur->ai_addrlen ) == 0 )
            {
                nRet = JIOT_SUCCESS;
                break;
            }
            else
            {
                nRet = JIOT_ERR_TCP_SOCKET_CONN;
                
                close( fd );
                fd = -1 ;
                continue;
            }
        }
        else
        {
            close( fd );
            fd = -1 ;
            continue;
        }
    }

    if(nRet != JIOT_SUCCESS )
    {
        nRet = JIOT_ERR_TCP_SOCKET_CONN;
    }
    
    freeaddrinfo(addrInfoList);

    *pfd = fd;
    return nRet;
}


