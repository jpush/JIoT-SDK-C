#include "jiot_network_ssl.h"
#include "jiot_network.h"
#include "jiot_timer.h"
#include "jiot_log.h"

#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>


#ifdef JIOT_SSL

static SSL_CTX *ssl_ctx = NULL;
static X509_STORE *ca_store = NULL;
static X509 *ca = NULL;

static X509 *ssl_load_cert(const char *cert_str)
{
    X509 *cert = NULL;
    BIO *in = NULL;
	
    if(!cert_str) 
	{
        return NULL;
    }

    in = BIO_new_mem_buf((void *)cert_str, -1);
    if(!in) 
	{
		ERROR_LOG("openssl malloc cert buffer failed");
        return NULL;
    }

    cert = PEM_read_bio_X509(in, NULL, NULL, NULL);
    if(in)
	{
		//ERROR_LOG("openssl read pem failed");
        BIO_free(in);
		//in = NULL;
    }
	
    return cert;
}


static int ssl_ca_store_init(const char *my_ca )
{
    if(!ca_store)
	{
        if(!my_ca)
		{
            return JIOT_FAIL;
        }
		
        ca_store = X509_STORE_new();
        ca = ssl_load_cert(my_ca);
        int ret = X509_STORE_add_cert(ca_store, ca);
        if(ret != 1)
		{
            return JIOT_FAIL;
        }
    }

    return JIOT_SUCCESS;
}

//extern int OPENSSL_init_ssl();
static int ssl_init( const char *my_ca )
{
    if(ssl_ca_store_init(my_ca) != 0)
    {
        return JIOT_FAIL;
    }

    if(!ssl_ctx)
    {
        const SSL_METHOD *meth;

        SSLeay_add_ssl_algorithms();

        meth = TLSv1_2_client_method();

        SSL_load_error_strings();
        ssl_ctx = SSL_CTX_new(meth);
        if(!ssl_ctx)
        {
            return JIOT_FAIL;
        }
    }
    
    return JIOT_SUCCESS;
}

#ifdef JIOT_SSL_VERIFY_CA
static int ssl_verify_ca(X509 *target_cert)
{
    STACK_OF(X509) *ca_stack = NULL;
    X509_STORE_CTX *store_ctx = NULL;
    int ret;

    store_ctx = X509_STORE_CTX_new();

    ret = X509_STORE_CTX_init(store_ctx, ca_store, target_cert, ca_stack);

    if (ret != 1) {
        ERROR_LOG("X509_STORE_CTX_init fail, ret = %d", ret);
        goto end;
    }
    ret = X509_verify_cert(store_ctx);
    if (ret != 1) {
    	ERROR_LOG("X509_verify_cert fail, ret = %d, error id = %d, %s\n",
               ret, store_ctx->error,
               X509_verify_cert_error_string(store_ctx->error));
        goto end;
    }
end:
    if (store_ctx) {
        X509_STORE_CTX_free(store_ctx);
    }

    return (ret == 1) ? 0 : -1;

}
#endif

static int ssl_establish(int sock, SSL **ppssl)
{
    int err;
    SSL *ssl_temp = NULL;
    X509 *server_cert = NULL;

    if(!ssl_ctx)
    {
        ERROR_LOG("no ssl context to create ssl connection \n");
        return JIOT_FAIL;
    }

    ssl_temp = SSL_new(ssl_ctx);

    SSL_set_fd(ssl_temp, sock);
    err = SSL_connect(ssl_temp);
    if(err == -1)
    {
        ERROR_LOG("failed create ssl connection \n");
        goto err;
    }

#ifdef JIOT_SSL_VERIFY_CA
    server_cert = SSL_get_peer_certificate(ssl_temp);
    if (!server_cert) {
        ERROR_LOG("failed to get server cert");
        goto err;
    }
    if (ssl_verify_ca(server_cert) != 0)
    {
        goto err;
    }
    ERROR_LOG("success to verify cert \n");
#endif

    *ppssl = ssl_temp;

    return JIOT_SUCCESS;

err:
    if(ssl_temp)
    {
        SSL_free(ssl_temp);
    }
	
    if(server_cert)
    {
        X509_free(server_cert);
    }

    *ppssl = NULL;
    return JIOT_FAIL;
}

SSL *platform_ssl_connect(int tcp_fd, const char *server_cert, int server_cert_len)
{
    SSL *pssl;

    if(0 != ssl_init( server_cert ))
    {
        return NULL;
    }

    if(0 != ssl_establish(tcp_fd, &pssl))
    {
        return NULL;
    }

    return pssl;
}


int platform_ssl_close(SSL *ssl)
{
    if(ssl)
    {
        SSL_set_shutdown(ssl, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
        SSL_free(ssl);
        ssl = NULL;
    }

    if(ssl_ctx)
    {
        SSL_CTX_free(ssl_ctx);
        ssl_ctx = NULL;
    }

    if(ca)
    {
        X509_free(ca);
        ca = NULL;
    }

    if(ca_store)
    {
        X509_STORE_free(ca_store);
        ca_store = NULL;
    }

    return JIOT_SUCCESS;
}

int jiot_network_tcp_connect(TLSDataParams *pTlsData, const char *addr, const char *port,E_NET_TYPE nType)
{
    /* create client socket */
    int nRet = jiot_network_create(&(pTlsData->socketId),addr,port,nType);
    if(nRet != JIOT_SUCCESS )
    {
        pTlsData->socketId = -1;
        return nRet;
    }
    
    return JIOT_SUCCESS;
}

//读取缓冲区中指定长度的数据。
int jiot_network_ssl_read(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms)
{
	int readLen = 0;
	int ret = -1;

    S_JIOT_TIME_TYPE endTime;
    jiot_timer_countdown(&endTime,timeout_ms);

    if (pTlsData->pssl == NULL)
    {
        return JIOT_ERR_SSL_READ_FAIL;
    }

	if(!SSL_pending(pTlsData->pssl)) 
	{
		ret = jiot_network_select(pTlsData->socketId, TRANS_RECV, timeout_ms);

		if ((ret == JIOT_ERR_TCP_SELECT_TIMEOUT)||(ret == JIOT_ERR_TCP_RETRY))
        {
            return JIOT_ERR_SSL_SELECT_TIMEOUT;
        }
        else
        if(ret == JIOT_FAIL)
        {
            return JIOT_ERR_SSL_SELECT_FAIL;
        }    
	}

	while (readLen < len) 
	{
        INT32 lefttime = jiot_timer_remain(&endTime);
        if(lefttime <= 0)
        {
            return JIOT_ERR_SSL_READ_TIMEOUT;
        }

        jiot_network_settimeout(pTlsData->socketId,TRANS_RECV,500);

        if (pTlsData->pssl == NULL )
        {
            return  JIOT_ERR_SSL_READ_FAIL;
        }

		ret = SSL_read(pTlsData->pssl, (unsigned char *)(buffer + readLen), (len - readLen));
		if (ret > 0) 
		{
			readLen += ret;
		} 
		else if (ret == 0) 
		{
            if (SSL_get_error(pTlsData->pssl,ret) == SSL_ERROR_ZERO_RETURN)
            {
                //SSL Close by peer 
				DEBUG_LOG("SSL READ FAIL ");
                return JIOT_ERR_SSL_READ_FAIL;
            }

			return JIOT_ERR_SSL_READ_TIMEOUT; //eof
		} 
		else 
		{
            DEBUG_LOG("SSL READ FAIL %d",ret);
			return JIOT_ERR_SSL_READ_FAIL; //Connnection error
		}
	}

	return readLen;
}

//读取缓冲区数据，指定最大值。
int jiot_network_ssl_read_buffer(TLSDataParams *pTlsData, unsigned char *buffer, int maxlen, int timeout_ms)
{
    int readLen = 0;
    int ret = -1;

    if(!SSL_pending(pTlsData->pssl)) 
    {
        ret = jiot_network_select(pTlsData->socketId, TRANS_RECV, timeout_ms);
        if ((ret == JIOT_ERR_TCP_SELECT_TIMEOUT)||(ret == JIOT_ERR_TCP_RETRY))
        {
            return JIOT_ERR_SSL_SELECT_TIMEOUT;
        }
        else
        if(ret == JIOT_FAIL)
        {
            return JIOT_ERR_SSL_SELECT_FAIL;
        } 
    }

    jiot_network_settimeout(pTlsData->socketId,TRANS_RECV,500);
    ret =  SSL_read(pTlsData->pssl, (unsigned char *)buffer , maxlen );
    if (ret > 0) 
    {
        readLen += ret;
    } 
    else if (ret == 0) 
    {
        return JIOT_ERR_SSL_READ_TIMEOUT; //eof
    } 
    else 
    {
        DEBUG_LOG("SSL READ FAIL %d",ret);
        return JIOT_ERR_SSL_READ_FAIL; //Connnection error
    }

    return readLen;
}

int jiot_network_ssl_write(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms)
{
	int writtenLen = 0;
	int ret = 0;

	while (writtenLen < len) 
	{
        ret = jiot_network_select(pTlsData->socketId, TRANS_SEND, timeout_ms);
        if ((ret == JIOT_ERR_TCP_SELECT_TIMEOUT)||(ret == JIOT_ERR_TCP_RETRY))
        {
            return JIOT_ERR_SSL_SELECT_TIMEOUT;
        }
        else
        if(ret == JIOT_FAIL)
        {
            return JIOT_ERR_SSL_SELECT_FAIL;
        } 
        
        jiot_network_settimeout(pTlsData->socketId,TRANS_SEND,500);
              
        ret = SSL_write(pTlsData->pssl, (unsigned char *)(buffer + writtenLen), (len - writtenLen));
        if (ret > 0) 
        {
            writtenLen += ret;
            continue;
        } 
        else if (ret == 0) 
        {
            return writtenLen;
        } 
        else 
        {
            DEBUG_LOG("SSL READ FAIL %d",ret);
            return JIOT_ERR_SSL_WRITE_FAIL; //Connnection error
        }
        
	}
	return writtenLen;
}

void jiot_network_ssl_disconnect(TLSDataParams *pTlsData)
{
	if(pTlsData->pssl != NULL)
	{
		platform_ssl_close(pTlsData->pssl);
		pTlsData->pssl = NULL;
	}
	
	if(pTlsData->socketId >= 0)
	{
		close(pTlsData->socketId);
		pTlsData->socketId = -1;
	}
}

int jiot_network_ssl_connect(TLSDataParams *pTlsData, const char *addr, const char *port, const char *ca_crt, int ca_crt_len,E_NET_TYPE nType)
{
    int nRet = JIOT_SUCCESS;

	if(NULL == pTlsData)
	{
		return JIOT_ERR_SSL_IS_NULL;
	}
    
    nRet = jiot_network_tcp_connect(pTlsData,addr,port,nType);

    if (nRet != JIOT_SUCCESS)
    {
        return JIOT_ERR_SSL_SOCKET_CREATE_FAIL;
    }

    pTlsData->pssl = platform_ssl_connect(pTlsData->socketId, ca_crt, ca_crt_len);
    if(NULL == pTlsData->pssl)
    {
        return JIOT_ERR_SSL_CONNECT_FAIL;
    }

    return JIOT_SUCCESS;

}

#endif

