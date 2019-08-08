#include "jiot_tcp_net.h"
#include "jiot_network.h"
#include "jiot_pthread.h"
#include "jiot_err.h"
#include "jiot_timer.h"


int ConnectTcpNetwork(Network* pNetwork, char* addr, char* port)
{
    int nRet = jiot_network_create(&pNetwork->_socket ,addr,port,pNetwork->connectparams.eNetworkType);
    if(nRet != JIOT_SUCCESS )
    {
        return nRet;
    }
    
    return JIOT_SUCCESS;
}

//读取缓冲区中指定长度的数据。
int jiot_tcp_read(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms)
{
    int rc = 0;
    int recvlen = 0;
    int ret = -1;

    S_JIOT_TIME_TYPE endTime;
    jiot_timer_countdown(&endTime,timeout_ms);
    do
    {
        INT32 lefttime = jiot_timer_remain(&endTime);
        if(lefttime <= 0)
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }

        ret = jiot_network_select(pNetwork->_socket,TRANS_RECV,lefttime);
        
        if (ret == JIOT_ERR_TCP_RETRY)
        {
            continue ;
        }
        else if (ret == JIOT_FAIL)
        {
            return JIOT_ERR_NET_SELECT_FAIL;
        }
        else if (ret == JIOT_ERR_TCP_SELECT_TIMEOUT)
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }
        else 
        {
            jiot_network_settimeout(pNetwork->_socket,TRANS_RECV,500);

            rc = jiot_network_recv(pNetwork->_socket, buffer + recvlen, len - recvlen, TRANS_FLAGS_DEFAULT);
            if((rc == 0)||(rc == JIOT_FAIL))
            {
                recvlen = 0;
                return JIOT_ERR_NET_RECV_FAIL;
            }
            else if (rc == JIOT_ERR_TCP_RETRY)
            {
                continue;
            }
            else
            {
                //读取缓冲区数据成功
                recvlen += rc;
            }
        }

    }while(recvlen < len);

    return recvlen;
}

//读取缓冲区数据，指定最大值。
int jiot_tcp_read_buffer(Network *pNetwork, unsigned char *buffer, int maxlen, int timeout_ms)
{
    int rc = 0;
    int recvlen = 0;
    int ret = -1;

    S_JIOT_TIME_TYPE endTime;
    jiot_timer_countdown(&endTime,timeout_ms);
    do
    {
        INT32 lefttime = jiot_timer_remain(&endTime);
        if(lefttime <= 0)
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }

        ret = jiot_network_select(pNetwork->_socket,TRANS_RECV,lefttime);
        if (ret == JIOT_ERR_TCP_RETRY)
        {
            continue ;
        }
        else if (ret == JIOT_FAIL)
        {
            return JIOT_ERR_NET_SELECT_FAIL;
        }
        else if (ret == JIOT_ERR_TCP_SELECT_TIMEOUT)
        {
            return JIOT_ERR_NET_READ_TIMEOUT;
        }
        else 
        {
            jiot_network_settimeout(pNetwork->_socket,TRANS_RECV,500);

            rc = jiot_network_recv(pNetwork->_socket, buffer , maxlen , TRANS_FLAGS_DEFAULT);
            if((rc == 0)||(rc == JIOT_FAIL))
            {
                recvlen = 0;
                return JIOT_ERR_NET_RECV_FAIL;
            }
            else if (rc == JIOT_ERR_TCP_RETRY)
            {
                continue;
            }
            else
            {
                //读取缓冲区数据成功
                recvlen = rc;

            }
        }

    }while(recvlen < 0);

    return recvlen;
}


int jiot_tcp_write(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms)
{
    int rc = 0;
    int ret = -1;
    INT32 sendlen = 0;

    INT32 timeout = timeout_ms;
    S_JIOT_TIME_TYPE endTime;
    jiot_timer_countdown(&endTime,timeout);

    do
    {
        INT32 lefttime = jiot_timer_remain(&endTime);
        if(lefttime <= 0)
        {
            return JIOT_ERR_NET_WRITE_TIMEOUT;
        }

        ret = jiot_network_select(pNetwork->_socket,TRANS_SEND,lefttime);
        if (ret == JIOT_ERR_TCP_RETRY)
        {
            continue ;
        }
        else if (ret == JIOT_FAIL)
        {
            return JIOT_ERR_NET_SELECT_FAIL;
        }
        else if (ret == JIOT_ERR_TCP_SELECT_TIMEOUT)
        {
            return JIOT_ERR_NET_WRITE_TIMEOUT;
        }
        else 
        {
            jiot_network_settimeout(pNetwork->_socket,TRANS_SEND,500);

            rc = jiot_network_send(pNetwork->_socket, buffer, len);
            if(rc == JIOT_ERR_TCP_RETRY)
            {
                continue;

            }
            else if((rc == 0)||(rc == JIOT_FAIL))
            {
                return JIOT_ERR_NET_WRITE_FAIL;
            }
            else
            {
                sendlen += rc;
            }
        }
    }while(sendlen < len);

    return sendlen;
}

void jiot_tcp_disconnect(Network *pNetwork)
{
    if( pNetwork->_socket < 0 )
        return;
    
    jiot_network_close(pNetwork->_socket);

    pNetwork->_socket = -1;
}

int jiot_tcp_connect(Network *pNetwork)
{
    if(NULL == pNetwork)
    {
        return JIOT_ERR_NET_IS_NULL;
    }

    int nRet = ConnectTcpNetwork(pNetwork,pNetwork->connectparams.pHostAddress, pNetwork->connectparams.pHostPort);
    if (nRet != JIOT_SUCCESS)
    {
        pNetwork->_socket = -1;
        return JIOT_ERR_NET_CREATE_FAIL;
    }
    
    return JIOT_SUCCESS;
}

