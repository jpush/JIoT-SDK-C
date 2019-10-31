#include "jiot_network_ssl.h"
#include "jiot_timer.h"
#include "jiot_pthread.h"
#include "jiot_log.h"


#ifdef JIOT_SSL


int ssl_client_init(mbedtls_ssl_context *ssl,
                         mbedtls_net_context *tcp_fd,
                         mbedtls_ssl_config *conf,
                         mbedtls_x509_crt *crt509_ca, const char *ca_crt, size_t ca_len,
                         mbedtls_x509_crt *crt509_cli, const char *cli_crt, size_t cli_len,
                         mbedtls_pk_context *pk_cli, const char *cli_key, size_t key_len,  const char *cli_pwd, size_t pwd_len
                        )
{
    mbedtls_net_init(tcp_fd);
    mbedtls_ssl_init(ssl);
    mbedtls_ssl_config_init(conf);
    return 0;
}

static int ssl_random(void *p_rng, unsigned char *output, size_t output_len)
{
    uint32_t rnglen = output_len;
    uint8_t  rngoffset = 0;

    while (rnglen > 0)
    {
        *(output + rngoffset) = (unsigned char)(((unsigned int)rand() << 16) + rand());
        rngoffset++;
        rnglen--;
    }
    return 0;
}


int TLSConnectNetwork(TLSDataParams *pTlsData, const char *addr, const char *port,
                      const char *ca_crt, size_t ca_crt_len,
                      const char *client_crt,   size_t client_crt_len,
                      const char *client_key,   size_t client_key_len,
                      const char *client_pwd, size_t client_pwd_len)
{
    int ret = -1;
    /*
     * 0. Init
     */
    if (0 != (ret = ssl_client_init(&(pTlsData->ssl), &(pTlsData->fd), &(pTlsData->conf),
                                         &(pTlsData->cacertl), ca_crt, ca_crt_len,
                                         &(pTlsData->clicert), client_crt, client_crt_len,
                                         &(pTlsData->pkey), client_key, client_key_len, client_pwd, client_pwd_len)))
    {
        ERROR_LOG( "ssl_client_init returned -0x%04x", -ret );
        return JIOT_ERR_SSL_SOCKET_CREATE_FAIL;
    }

    /*
     * 1. Start the connection
     */
    DEBUG_LOG("Connecting to tcp/%s/%s", addr, port);
    if (0 != (ret = mbedtls_net_connect(&(pTlsData->fd), addr, port, MBEDTLS_NET_PROTO_TCP)))
    {
        ERROR_LOG("net_connect returned -0x%04x", -ret);
        return JIOT_ERR_SSL_SOCKET_CREATE_FAIL;
    }
    pTlsData->socketId = pTlsData->fd.fd;
    DEBUG_LOG("Connected socketId=%d", pTlsData->socketId);

    /*
     * 2. Setup stuff
     */
    if ( ( ret = mbedtls_ssl_config_defaults( &(pTlsData->conf),MBEDTLS_SSL_IS_CLIENT,MBEDTLS_SSL_TRANSPORT_STREAM,MBEDTLS_SSL_PRESET_DEFAULT ) ) != 0 )
    {
        ERROR_LOG( "mbedtls_ssl_config_defaults returned %d", ret );
        return JIOT_ERR_SSL_CONNECT_FAIL;
    }
    

    /* OPTIONAL is not optimal for security, but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode( &(pTlsData->conf), MBEDTLS_SSL_VERIFY_NONE);    
    mbedtls_ssl_conf_rng( &(pTlsData->conf), ssl_random, NULL );

    if ( ( ret = mbedtls_ssl_setup(&(pTlsData->ssl), &(pTlsData->conf)) ) != 0 )
    {
        ERROR_LOG( "mbedtls_ssl_setup returned %d", ret );
        return JIOT_ERR_SSL_CONNECT_FAIL;
    }
    mbedtls_ssl_set_hostname(&(pTlsData->ssl), addr);
    mbedtls_ssl_set_bio( &(pTlsData->ssl), &(pTlsData->fd), mbedtls_net_send, mbedtls_net_recv, mbedtls_net_recv_timeout);

    /*
      * 4. Handshake
      */
    while ((ret = mbedtls_ssl_handshake(&(pTlsData->ssl))) != 0)
    {
        if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE))
        {
            ERROR_LOG( "mbedtls_ssl_handshake returned -0x%04x", -ret);
            return JIOT_ERR_SSL_CONNECT_FAIL;
        }
    }

    /*
     * 5. Verify the server certificate
     */

    // 当前版本不需要验证CA证书
    /*
    DEBUG_LOG("  . Verifying peer X.509 certificate..");
    if (0 != mbedtls_ssl_get_verify_result(&(pTlsData->ssl)))
    {
        ERROR_LOG(" failed  ! verify result not confirmed.");
        return JIOT_ERR_SSL_CONNECT_FAIL;
    }
    */

    DEBUG_LOG("Connected success");
    return JIOT_SUCCESS;
}

int jiot_network_ssl_read(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms)
{
    size_t readLen = 0;
    int ret = -1;

    S_JIOT_TIME_TYPE nowTime;
	jiot_timer_now_clock(&nowTime);
    mbedtls_ssl_conf_read_timeout(&(pTlsData->conf), timeout_ms);
    while (readLen < len)
    {
        ret = mbedtls_ssl_read(&(pTlsData->ssl), (unsigned char *)(buffer + readLen), (len - readLen));
        if (ret > 0)
        {
            readLen += ret;
        }
        else if (ret == 0 || ret == MBEDTLS_ERR_SSL_TIMEOUT)
        {
            //释放线程的时间片，防止线程调度异常
            int spend_time_ms = jiot_timer_spend(&nowTime);
			if(timeout_ms - spend_time_ms > 0){
				jiot_sleep(5);
			}
            return JIOT_ERR_SSL_READ_TIMEOUT; //eof
        }
        else
        {   
            DEBUG_LOG("SSL READ FAIL  -0x%x",-ret);
            return JIOT_ERR_SSL_READ_FAIL; //Connnection error
        }
    }
    return readLen;
}

int jiot_network_ssl_read_buffer(TLSDataParams *pTlsData, unsigned char *buffer, int maxlen, int timeout_ms)
{

    mbedtls_ssl_conf_read_timeout(&(pTlsData->conf), timeout_ms);
	int ret = mbedtls_ssl_read(&(pTlsData->ssl), (unsigned char *)(buffer), maxlen);
	
    if(ret > 0)
    {
        return ret;
    }
    else if(ret == 0 || ret == MBEDTLS_ERR_SSL_TIMEOUT)
    {
        return JIOT_ERR_SSL_READ_TIMEOUT;
    }

    DEBUG_LOG("SSL READ FAIL -0x%x",-ret);
    return JIOT_ERR_SSL_READ_FAIL;
}

int jiot_network_ssl_write(TLSDataParams *pTlsData, unsigned char *buffer, int len, int timeout_ms)
{
    size_t writtenLen = 0;
    int ret = -1;

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
              

        ret = mbedtls_ssl_write(&(pTlsData->ssl), (unsigned char *)(buffer + writtenLen), (len - writtenLen));
       
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
            return JIOT_ERR_SSL_WRITE_FAIL; //Connnection error
        }

    }
    return writtenLen;
}

void jiot_network_ssl_disconnect(TLSDataParams *pTlsData)
{
	DEBUG_LOG("ssl and network close begin");
    mbedtls_ssl_close_notify(&(pTlsData->ssl));
    mbedtls_net_free(&(pTlsData->fd));
    mbedtls_ssl_free(&(pTlsData->ssl));
    mbedtls_ssl_config_free(&(pTlsData->conf));
    DEBUG_LOG("ssl and network close end");
}

int jiot_network_ssl_connect(TLSDataParams *pTlsData, const char *addr, const char *port, const char *ca_crt, int ca_crt_len,E_NET_TYPE nType)
{
    return TLSConnectNetwork(pTlsData, addr, port, ca_crt, ca_crt_len, NULL, 0, NULL, 0, NULL, 0);
}

#endif

