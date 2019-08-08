#include "jiot_ssl_net.h"
#include "jiot_network_ssl.h"
#include "jiot_network_define.h"

#ifdef JIOT_SSL

//读取缓冲区中指定长度的数据。
int jiot_ssl_read(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms)
{

    if(NULL == pNetwork)
    {
        return JIOT_ERR_NET_IS_NULL;
    }

    int nRet = jiot_network_ssl_read(&pNetwork->tlsdataparams, buffer, len, timeout_ms);

    if ((nRet ==JIOT_ERR_SSL_SELECT_TIMEOUT)||(nRet == JIOT_ERR_SSL_READ_TIMEOUT))
    {
        return JIOT_ERR_NET_READ_TIMEOUT;
    }
    else
    if ((nRet ==JIOT_ERR_SSL_SELECT_FAIL)||(nRet == JIOT_ERR_SSL_READ_FAIL))
    {
        return JIOT_ERR_NET_RECV_FAIL;
    }
    return nRet ;

}

//读取缓冲区数据，指定最大值。
int jiot_ssl_read_buffer(Network *pNetwork, unsigned char *buffer, int maxlen, int timeout_ms)
{
    if(NULL == pNetwork)
    {
        return JIOT_ERR_NET_IS_NULL;
    }
    
    int nRet =  jiot_network_ssl_read_buffer(&pNetwork->tlsdataparams, buffer, maxlen, timeout_ms);

    if ((nRet == JIOT_ERR_SSL_SELECT_TIMEOUT)||(nRet == JIOT_ERR_SSL_READ_TIMEOUT))
    {
        return JIOT_ERR_NET_READ_TIMEOUT;
    }
    else
    if ((nRet == JIOT_ERR_SSL_SELECT_FAIL)||(nRet == JIOT_ERR_SSL_READ_FAIL))
    {
        return JIOT_ERR_NET_RECV_FAIL;
    }
    return nRet ;
}

int jiot_ssl_write(Network *pNetwork, unsigned char *buffer, int len, int timeout_ms)
{
    if(NULL == pNetwork)
    {
        return JIOT_ERR_NET_IS_NULL;
    }

    int nRet =   jiot_network_ssl_write(&pNetwork->tlsdataparams, buffer, len, timeout_ms);
    if ((nRet == JIOT_ERR_SSL_SELECT_TIMEOUT))
    {
        return JIOT_ERR_NET_WRITE_TIMEOUT;
    }
    else
    if ((nRet == JIOT_ERR_SSL_SELECT_FAIL)||(nRet == JIOT_ERR_SSL_WRITE_FAIL))
    {
        return JIOT_ERR_NET_WRITE_FAIL;
    }
    return nRet ;
}

void jiot_ssl_disconnect(Network *pNetwork)
{
    if(NULL == pNetwork||pNetwork->tlsdataparams.socketId  < 0)
    {
        return ;
    }

    jiot_network_ssl_disconnect(&pNetwork->tlsdataparams);

    pNetwork->tlsdataparams.socketId = -1;

    pNetwork->_socket = pNetwork->tlsdataparams.socketId ;
    
}

int jiot_ssl_connect(Network *pNetwork)
{
    if(NULL == pNetwork)
    {
        return JIOT_ERR_NET_IS_NULL;
    }

    int nRet = jiot_network_ssl_connect(&pNetwork->tlsdataparams, pNetwork->connectparams.pHostAddress, pNetwork->connectparams.pHostPort,
                             pNetwork->connectparams.pPubKey, strlen(pNetwork->connectparams.pPubKey)+1,pNetwork->connectparams.eNetworkType);

    if (nRet != JIOT_SUCCESS)
    {
        pNetwork->tlsdataparams.socketId = -1;
        pNetwork->_socket = pNetwork->tlsdataparams.socketId ;
        
        return JIOT_ERR_NET_CREATE_FAIL;
    }

    pNetwork->_socket = pNetwork->tlsdataparams.socketId ;

    return JIOT_SUCCESS;
}

#endif
