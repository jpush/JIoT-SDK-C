
#include "jiot_cJSON.h"
#include "jiot_httpclient.h"
#include "jiot_sis_client.h"
#include "jiot_common.h"
#include "jiot_code.h"


char globalSisHost[SIS_HOST_MAX_LEN]   = "sis.iot.jiguang.cn";
char globalSisIpAddr[SIS_HOST_MAX_LEN] = "113.31.131.88";

char sis_ca[] = "-----BEGIN CERTIFICATE-----\n\
MIIDhzCCAm+gAwIBAgIJANgYJ0pT28iqMA0GCSqGSIb3DQEBCwUAMFoxCzAJBgNV\n\
BAYTAkNOMQswCQYDVQQIDAJHRDELMAkGA1UEBwwCU1oxCzAJBgNVBAoMAkpHMQww\n\
CgYDVQQLDANJT1QxFjAUBgNVBAMMDTExMy4zMS4xMzEuNTkwHhcNMTgxMDMxMDgx\n\
NTA4WhcNMTkxMDMxMDgxNTA4WjBaMQswCQYDVQQGEwJDTjELMAkGA1UECAwCR0Qx\n\
CzAJBgNVBAcMAlNaMQswCQYDVQQKDAJKRzEMMAoGA1UECwwDSU9UMRYwFAYDVQQD\n\
DA0xMTMuMzEuMTMxLjU5MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA\n\
0iqo1Jibg40W6wqXCwg6RpYS8cbfOStdjgyEyU1rPq0wLlND3R0DJ7GI83uWKjyl\n\
2q1lKD93Ep/lXVbBTplBixcHfxBzqJFYCIUz8IxczTT0EwSio1mnBVZuhexuH4WS\n\
Tg1Or1QRni8w5n62g8x8KrgXQu/8WvxoU1lH8MRDZFSZG/llKmNjH7y2UTRwYCmO\n\
dpN6RBEQzxoYSgy9FujR5ZGaY7d/HM5RdjfFSwtXyHKXxVPF+Mm7pF4GyxyvF4j+\n\
yBdYAx5BpW5S91PjXNreuiUnLsJp7dGU0TfjI3j0gQ6YlUlqrBnrAcoU7oRCMRH6\n\
tWNTsY5519JLvx9uDZhOKQIDAQABo1AwTjAdBgNVHQ4EFgQUsxdkasTPJemeLVGx\n\
RmzypVX0HuQwHwYDVR0jBBgwFoAUsxdkasTPJemeLVGxRmzypVX0HuQwDAYDVR0T\n\
BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEAIfV2Wn+XRUe2WUFPXqf+Sw7QhGmM\n\
Xwy6Jhl1WJ3ag7FqhfUEGeoSyRK4en4tqrqcDJcWbMHCki4yh36C0GSbEaX4QobD\n\
hCr0lWagEwkxwp4tHDwqZ6m0lmujazG+/sVw8KINqPdAj/s27mggxteflfj9+qQO\n\
3K/MNI6KQZnuevISlhagkev+enj6RwG2XEC/FkrQv7Gv+jE2FUQwvmRfZVpLK2jv\n\
OHatRqY5b4pNpytNteZ3dKRJVudWI9M06YnwWSFREAUOr0iXhQiCoG+ne2KPDaOS\n\
Jr/jgJtk76t95mpQx6GnnfJCyiFPtxyBMNOd9fkrGLDxQG5Diu/L49XiGQ==\n\
-----END CERTIFICATE-----\n";


void genSisUrl(char* sisUrl, int maxlen, const char* host, const char* uri, const char* productKey)
{
#ifdef JIOT_SSL
    snprintf(sisUrl, maxlen - 1, "https://%s/%s?product_key=%s&protocol_type=%d", host, uri, productKey, PROTOCOL_TYPE_SSL);
#else
    snprintf(sisUrl, maxlen - 1, "http://%s/%s?product_key=%s&protocol_type=%d", host, uri, productKey, PROTOCOL_TYPE_TCP);
#endif
}


int connSis(httpclient_t *client, char * url, const char *ca_crt)
{
    int port = 0;
    char scheme[8] = {0};
    char host[HTTPCLIENT_MAX_HOST_LEN] ={0};

    jiot_httpclient_parse_host(url, scheme, sizeof(scheme), host, sizeof(host),&port);

    DEBUG_LOG("conn sis server host: '%s://%s',  remote port[%d]", scheme, host, port);

    char szPort[4] = {0};
    sprintf(szPort, "%d", port);

    int ret = jiot_http_network_init(&client->net, host, szPort, (char*)ca_crt);
    if (JIOT_SUCCESS != ret) 
    {
        return ret;
    }

    ret = jiot_httpclient_connect(client);
    if (JIOT_SUCCESS != ret) 
    {
        ERROR_LOG("httpclient_connect is error, ret = %d", ret);
        jiot_httpclient_close(client);
    }

    return ret;
}

int requestSis(httpclient_t *client, const char *url, HTTPCLIENT_REQUEST_TYPE method, UINT32 timeout_ms, httpclient_data_t *client_data)
{
    Timer timer;
    int ret = JIOT_SUCCESS;

    if (client->net._socket >= 0) 
    {
        ret = jiot_httpclient_send_request(client, url, method, client_data);
        if (0 != ret) 
        {
            ERROR_LOG("httpclient_send_request is error, ret = %d", ret);
            jiot_httpclient_close(client);
            return JIOT_ERR_SIS_HTTP_REQ_FAIL;
        }
    }

    InitTimer(&timer);
    countdown_ms(&timer, timeout_ms);

    if ((NULL != client_data->response_buf)
        && (0 != client_data->response_buf_len)) 
    {
        ret = jiot_httpclient_recv_response(client, left_ms(&timer), client_data);
        if (ret < 0) 
        {
            ERROR_LOG("httpclient_recv_response is error,ret = %d", ret);
            jiot_httpclient_close(client);
            return JIOT_ERR_SIS_HTTP_FAIL;
        }
    }

    if (! client_data->is_more) 
    {
        /* Close the HTTP if no more data. */
        DEBUG_LOG("close http channel");
        jiot_httpclient_close(client);
    }

    return JIOT_SUCCESS;
}

int parseSis(httpclient_data_t *client_data, SisInfo* sis)
{
    memset(sis, 0, sizeof(SisInfo));
    cJiotJSON * root = cJiotJSON_Parse(client_data->response_buf);
    cJiotJSON * obj  = NULL;
    obj = cJiotJSON_GetObjectItem(root,"hub_addr1");

    if(NULL == obj)
    {
        ERROR_LOG("json parse failed, content[%s]", client_data->response_buf);
        return JIOT_ERR_SIS_JSON_PARSE_FAIL;
    }

    int valid = 0;
    char* port = obj->valuestring;
	strcpy(sis->addr[0].host, strtok(port, ":"));
    if(NULL != port)
    {
        ++valid;
        sis->addr[0].port = atoi(strtok(NULL, ":"));
    }

    obj = cJiotJSON_GetObjectItem(root,"hub_addr2");
    port = obj->valuestring;
    strcpy(sis->addr[1].host, strtok(port, ":"));
    if(NULL != port)
    {
        ++valid;
        sis->addr[1].port = atoi(strtok(NULL, ":"));
    }
	/*
	//rt-thread平台编译不过
    strcpy(sis->addr[0].host, strsep(&port, ":"));
    if(NULL != port)
    {
        ++valid;
        sis->addr[0].port = atoi(port);
    }

    obj = cJiotJSON_GetObjectItem(root,"hub_addr2");
    port = obj->valuestring;
    strcpy(sis->addr[1].host, strsep(&port, ":"));
    if(NULL != port)
    {
        ++valid;
        sis->addr[1].port = atoi(port);
    }
	*/
    if(valid <= 0 )
    {
        ERROR_LOG("none ip could be used, valid[%d]", valid);
        return JIOT_ERR_SIS_CONTENT_ERROR;
    }

    cJiotJSON_Delete(root);
	return JIOT_SUCCESS;
}


int httpGetSis(httpclient_t * client,char * url,httpclient_data_t *client_data)
{
    int nRet =  JIOT_SUCCESS ;
    nRet = connSis(client,url,sis_ca);
    if (nRet != JIOT_SUCCESS)
    {
        return JIOT_ERR_SIS_HTTP_CONN_FAIL;
    }
    
    HTTPCLIENT_REQUEST_TYPE httpReqMethod = HTTPCLIENT_GET ;
    nRet = requestSis(client,url, httpReqMethod, 20*1000, client_data);
    if (nRet != JIOT_SUCCESS)
    {
        return JIOT_ERR_SIS_HTTP_FAIL;
    }

    return nRet ;
}

int getSisInfo(const SisDeviceInfo* dev, SisInfo* sis)
{
    httpclient_t client;
    httpclient_data_t client_data;
    memset(&client, 0, sizeof(httpclient_t));
    memset(&client_data, 0, sizeof(httpclient_data_t));
    char response_buf[SIS_RESP_MAX_LEN] = {0};
    client_data.response_buf = response_buf;
    client_data.response_buf_len = SIS_RESP_MAX_LEN;

    char sisUrl[SIS_URL_MAX_LEN] = {0};

    //using domain name connect
    genSisUrl(sisUrl, SIS_URL_MAX_LEN, globalSisHost, SIS_SERVICE_URI, dev->szProductKey);
    DEBUG_LOG("sisUrl[%s]", sisUrl);

    int retryTimes = 0, ret = -1, dataLen = 0;
    do
    {
        ret = httpGetSis(&client, sisUrl, &client_data);
        dataLen =strlen(client_data.response_buf);
        if(JIOT_SUCCESS != ret || dataLen < SIS_MIN_DATA_LEN)
        {
            ERROR_LOG("send http request failed ret[%d] dataLen[%d], retryTimes[%d]", ret, dataLen, retryTimes);
            ret = JIOT_ERR_SIS_HTTP_FAIL;
        }
        ret = parseSis(&client_data,sis);
    }
    while(JIOT_SUCCESS != ret && retryTimes++ < SIS_RETRY_TIMES);

    //using ip connect when connect fail
    if(JIOT_SUCCESS != ret)
    {
        memset(sisUrl, 0, SIS_RESP_MAX_LEN);
        
        genSisUrl(sisUrl, SIS_URL_MAX_LEN, globalSisIpAddr, SIS_SERVICE_URI, dev->szProductKey);
        
        DEBUG_LOG("domain connect error, using ip connect, sisUrl[%s]", sisUrl);
        
        ret = httpGetSis(&client, sisUrl, &client_data);
        if(JIOT_SUCCESS != ret)
        {
            ERROR_LOG("send http request failed ret[%d]", ret);
            return JIOT_ERR_SIS_HTTP_FAIL;
        }
        return  parseSis(&client_data,sis);
    }


    return JIOT_SUCCESS;
}
