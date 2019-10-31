/*
 * ota.c
 *
 *  Created on: Jul 9, 2019
 *      Author: szz
 */
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "jiot_client.h"


#include "jiot_tcp_net.h"
#include "jiot_httpclient.h"
#include <openssl/md5.h>

#define CURRENT_FIRMWARE_VERSION "v1.0.0"

typedef struct DEV
{
    long long seq_no;
    int desired_version ;
    int version ;
    int oldVersion;
    Property *pProperty;
    Event *pEvent;
    char app_ver[32];
    char msg[2048];
}DEV;

enum OTA_STEP
{
	OTA_STEP_INIT = 0,
	OTA_STEP_HTTP_DOWNLOAD_START = 1,
	OTA_STEP_HTTP_DOWNLOADING,
	OTA_STEP_HTTP_DOWNLOAD_FINASHED,
	OTA_STEP_FIRMWARE_CHECK,
	OTA_STEP_SYSTEM_REBOOT,
	OTA_STEP_HTTP_DOWNLOAD_FAILED,
	OTA_STEP_FIRMWARE_CHECK_FAILED,
};

struct httpOTACallbackParam
{
	int step;
	int downloadPercent; //[1-100]
	char* desc;
	long taskId;
};
typedef struct httpOTACallbackParam httpOTACallbackParam;
typedef int (*httpOTACallback)(httpOTACallbackParam* param);

struct httpOTAParam
{
	unsigned int firmwareSize;
	char* url;
	char* checksum;
	char* version;
	long taskId;
	httpOTACallback callback;
};
typedef struct httpOTAParam httpOTAParam;

int gHttpOTAProcessing = 0;
httpOTAParam gHttpOTAParam;
static JHandle gClienthandle = NULL;

#define OTA_BUFFER_SIZE 256

/* Derived from original code by CodesInChaos */
char *otaBinToHex(char *const hex, const size_t hex_maxlen,
              const unsigned char *const bin, const size_t bin_len)
{
    size_t i = (size_t)0U;
    unsigned int x;
    int b;
    int c;

    if (bin_len >= UINT32_MAX / 2 || hex_maxlen <= bin_len * 2U)
    {
        return NULL;
    }
    while (i < bin_len)
    {
        c = bin[i] & 0xf;
        b = bin[i] >> 4;
        x = (unsigned char)(87U + c + (((c - 10U) >> 8) & ~38U)) << 8 |
            (unsigned char)(87U + b + (((b - 10U) >> 8) & ~38U));
        hex[i * 2U] = (char)x;
        x >>= 8;
        hex[i * 2U + 1U] = (char)x;
        i++;
    }
    hex[i * 2U] = 0U;

    return hex;
}

int otaHexToBin(unsigned char *const bin, const size_t bin_maxlen,
            const char *const hex, const size_t hex_len,
            const char *const ignore, size_t *const bin_len,
            const char **const hex_end)
{
    size_t bin_pos = (size_t)0U;
    size_t hex_pos = (size_t)0U;
    int ret = 0;
    unsigned char c;
    unsigned char c_acc = 0U;
    unsigned char c_alpha0, c_alpha;
    unsigned char c_num0, c_num;
    unsigned char c_val;
    unsigned char state = 0U;

    while (hex_pos < hex_len)
    {
        c = (unsigned char)hex[hex_pos];
        c_num = c ^ 48U;
        c_num0 = (c_num - 10U) >> 8;
        c_alpha = (c & ~32U) - 55U;
        c_alpha0 = ((c_alpha - 10U) ^ (c_alpha - 16U)) >> 8;
        if ((c_num0 | c_alpha0) == 0U)
        {
            if (ignore != NULL && state == 0U && strchr(ignore, c) != NULL)
            {
                hex_pos++;
                continue;
            }
            break;
        }
        c_val = (c_num0 & c_num) | (c_alpha0 & c_alpha);
        if (bin_pos >= bin_maxlen)
        {
            ret = -1;
            break;
        }
        if (state == 0U)
        {
            c_acc = c_val * 16U;
        }
        else
        {
            bin[bin_pos++] = c_acc | c_val;
        }
        state = ~state;
        hex_pos++;
    }
    if (state != 0U)
    {
        hex_pos--;
    }
    if (hex_end != NULL)
    {
        *hex_end = &hex[hex_pos];
    }
    if (bin_len != NULL)
    {
        *bin_len = bin_pos;
    }
    return ret;
}


struct otaHttpClient
{
    httpclient_t client;
    httpclient_data_t data;
};
typedef struct otaHttpClient otaHttpClient;
extern char sis_ca[];

// ota platform interface
char gFirmwarePath[256] = {0};
int gFirmwareFd = -1;
int gExit = 0;
int otaFirmwareSaveInit(const char* postfix)
{

    readlink("/proc/self/exe", gFirmwarePath, sizeof(gFirmwarePath));
    strcat(gFirmwarePath, "-");
    strcat(gFirmwarePath, postfix);
    printf("otaFirmwareSaveInit path[%s]\n",gFirmwarePath);
    gFirmwareFd = open(gFirmwarePath, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if(gFirmwareFd < 0)
    {
        printf("otaFirmwareSaveInit open file failed\n");
        return -1;
    }
    return 0;
}
int otaFirmwareSaving(char* buf, unsigned int length)
{
	ssize_t ret = write(gFirmwareFd, buf, length);
	if(ret < 0)
	{
		printf("otaFirmwareSaving failed, ret[%ld]\n", ret);
		return -1;
	}
//    printf("otaFirmwareSaving written length[%d]\n", ret);
    return 0;
}
int otaFirmwareFinish()
{
	close(gFirmwareFd);
    return 0;
}
int otaSystemReboot()
{
	gExit = 1;
    return 0;
}

// http functions
otaHttpClient*  otaHttpGet(const char* url)
{
	otaHttpClient* httpClient = malloc(sizeof(otaHttpClient));
	if(NULL == httpClient)
	{
		return NULL;
	}
    memset(&(httpClient->client), 0, sizeof(httpclient_t));
    memset(&(httpClient->data), 0, sizeof(httpclient_data_t));

    httpClient->data.response_buf_len = 256;

    char* buf = malloc(httpClient->data.response_buf_len);
	if(NULL == buf)
	{
		printf("otaHttpGet buf malloc failed.\n");
		free(httpClient);
		return NULL;
	}
    httpClient->data.response_buf = buf;
    memset(httpClient->data.response_buf, 0, httpClient->data.response_buf_len);

    printf("otaHttpGet response_buf_len[%d]\n", httpClient->data.response_buf_len);


	int port = 0;
	char scheme[8] = {0};
	char host[HTTPCLIENT_MAX_HOST_LEN] ={0};

    jiot_httpclient_parse_host(url, scheme, sizeof(scheme), host, sizeof(host),&port);

    printf("otaHttpGet connect host: '%s://%s',  remote port[%d]\n", scheme, host, port);

	char szPort[4] = {0};
	sprintf(szPort, "%d", port);
	int ret = jiot_http_network_init(&(httpClient->client.net), host, szPort, sis_ca);

	if (0 != ret)
	{
		printf("jiot_http_network_init is error, ret = %d\n", ret);
        free(httpClient->data.response_buf);
        free(httpClient);
		return NULL;
	}

    ret = jiot_httpclient_connect(&(httpClient->client));
    if (0 != ret)
    {
        printf("otaHttpGet connect is error, ret = %d\n", ret);
        free(httpClient->data.response_buf);
        free(httpClient);
        return NULL;
    }
    printf("otaHttpConnect_connect success.\n");

    ret = jiot_httpclient_send_request(&(httpClient->client), url, HTTPCLIENT_GET, &(httpClient->data));
    if (0 != ret)
    {
        printf("httpclient_send_request is error, ret = %d\n", ret);
        jiot_httpclient_close(&(httpClient->client));
        free(httpClient->data.response_buf);
        free(httpClient);
        return NULL;
    }
    printf("otaHttpGet send request  success.\n");
    return httpClient;
}
int otaHttpFetch(otaHttpClient* httpClient, int timeoutMs)
{

	int received = 0;
	if ((NULL != httpClient->data.response_buf)
		&& (0 != httpClient->data.response_buf_len))
	{
		received = httpClient->data.retrieve_len;
		int ret = jiot_httpclient_recv_response(&(httpClient->client), timeoutMs, &(httpClient->data));
		if(0 == received && 0 != httpClient->data.response_content_len)
		{
			received = httpClient->data.response_content_len;
		}
		received = received - httpClient->data.retrieve_len;
		if (ret < 0)
		{
			printf("otaHttpFetch httpclient_recv_response is error, ret = %d\n", ret);
			jiot_httpclient_close(&(httpClient->client));
			return -1;
		}
	}
	return received;
}
unsigned int otaHttpNeedFetchMore(otaHttpClient* httpClient)
{
	if(NULL != httpClient)
	{
		return httpClient->data.is_more;
	}
	return 0;
}

void otaHttpFinish(otaHttpClient* httpClient)
{
	if(NULL != httpClient)
	{
		if(NULL != httpClient->data.response_buf)
		{
			free(httpClient->data.response_buf);
		}
		free(httpClient);
	}
}

// md5
int otaMD5Check(unsigned char* digest, size_t digestLen, const char* checksum)
{

    size_t checksumLen = strlen(checksum);
    unsigned char digest2[16] = {0};
    otaHexToBin(digest2, sizeof(digest2), checksum, checksumLen, NULL, NULL, NULL);

	char digestHexStr[33]={0};
	otaBinToHex(digestHexStr, sizeof(digestHexStr), digest, digestLen);
	printf("otaMD5Check firmware checked digest[%s] checksum[%s]\n", digestHexStr, checksum);

 	if(checksumLen != (digestLen * 2)
 			|| memcmp(digest, digest2, digestLen) != 0)
 	{
 		printf("otaMD5Check firmware checked failed");
 		return -1;
 	}
    printf("otaMD5Check md5Check pass\n");
    return 0;
}

// ota mqtt interface
int otaReportReq(httpOTACallbackParam* param)
{
//	printf("otaReportReq step[%d] percent[%d] desc %s\n", param->step, param->downloadPercent, param->desc);

	// report status
	OtaStatusReportReq req;
	req.desc = param->desc;
	req.seq_no = 0;
	req.step = 0;
	static int preDownloadPercent = 0;

	switch(param->step)
	{
	case OTA_STEP_INIT:
	case OTA_STEP_HTTP_DOWNLOAD_START:
		break;
	case OTA_STEP_HTTP_DOWNLOADING:
		req.step = param->downloadPercent;
		break;
	case OTA_STEP_HTTP_DOWNLOAD_FINASHED:
		req.step = 102;
		break;
	case OTA_STEP_SYSTEM_REBOOT:
		req.desc = "FIRMWARE_CHECK_SUCCEED";
		req.step = 103;
		break;
	case OTA_STEP_HTTP_DOWNLOAD_FAILED:
		req.step = -2;
		break;
	case OTA_STEP_FIRMWARE_CHECK_FAILED:
		req.step = -3;
		break;
	default:
		break;
	}

//	printf("otaReportReq #debug step[%d] percent[%d] preDownloadPercent[%d] report step[%d]\n", param->step, param->downloadPercent, preDownloadPercent, req.step);
	if(0 == req.step)
	{
		return 0;
	}
	if(param->step == OTA_STEP_HTTP_DOWNLOADING && (param->downloadPercent - preDownloadPercent < 20))
	{
		return 0;
	}
	if(param->step == OTA_STEP_HTTP_DOWNLOADING)
	{
		preDownloadPercent = param->downloadPercent;
	}

	printf("otaReportReq taskId[%ld] step[%d] percent[%d] report step[%d] desc %s\n", param->taskId, param->step, param->downloadPercent, req.step, req.desc);

	if(NULL == gClienthandle)
	{
		printf("otaReportReq clienthandle err");
		return -1;
	}

	JiotResult ret = jiotOtaStatusReportReq(gClienthandle, &req);
	if(ret.errCode == 0)
	{
//	    printf("ota report success\n");
	}
	else
	{
		printf("ota report err,code:[%d]\n",ret.errCode);
	}
	return 0;
}
int otaReportRsp(void* pContext,JHandle handle,const OtaStatusReportRsp * Rsp,int errcode)
{
	printf("otaReportRsp seqNo[%lld] errcode[%d]\n", Rsp->seq_no, errcode);
	return 0;
}

char otaHttpUrl[1024] = {0};
char otaChecksum[33] = {0};
char otaVersion[20] = {0};
int otaUpgradeReq(void* pContext,JHandle handle,OtaUpgradeInformReq *Req,int errcode)
{
    if (errcode != 0)
    {
        printf("otaUpgradeReq ota err code:[%d]\n",errcode);
    }

    printf("otaUpgradeReq:\n");
    printf("taskId[%ld] url[%s] md5[%s] version[%s] firmware size[%d]\n", Req->task_id, Req->url, Req->md5, Req->app_ver, Req->size);

    if(NULL == Req->url || NULL == Req->md5 || NULL == Req->app_ver || 0 == Req->size)
    {
    	return -1;
    }

    // ota start
	if(0 != gHttpOTAProcessing)
	{
		printf("ota already started, gHttpOTAProcessing[%d]\n", gHttpOTAProcessing);
		return -1;
	}
	else
	{
		printf("otaUpgradeReq ota processing\n");
		strcpy(otaHttpUrl, Req->url);
		strcpy(otaChecksum, Req->md5);
		strcpy(otaVersion, Req->app_ver);
		gHttpOTAParam.url = otaHttpUrl;
		gHttpOTAParam.checksum = otaChecksum;
		gHttpOTAParam.version = otaVersion;
		gHttpOTAParam.callback = otaReportReq;
		gHttpOTAParam.firmwareSize = Req->size;
		gHttpOTAParam.taskId = Req->task_id;
		gHttpOTAProcessing = 1;
	}
    return 0;
}

// jclient
int connectedHandle(void* pContext)
{
    printf("\nJclient connected \n");
    return 0;
}

int connectFailHandle(void* pContext,int errCode)
{
    printf("\nJclient connect fail . Code:[%d] \n",errCode);
    return 0;
}

int disconnectHandle(void* pContext)
{
    printf("\nJclient disconnect .  \n");
    return 0;
}

int subscribeFailHandle(void* pContext,char * TopicFilter)
{
    printf("\nJclient subscribe topic fail .Topic[%s] \n",TopicFilter);
    return 0;
}

int messageTimeoutHandle(void* pContext,long long seqNo)
{
    printf("\nJclient message time out seqNo[%lld]", seqNo);
    return 0;
}

int check(JHandle handle)
{
    while(1)
    {
        JClientStatus status = jiotGetConnStatus(handle);
        if(status != CLIENT_CONNECTED )
        {
            printf(".");
            fflush(stdout);
            sleep(1);
        }
        else
        {
            break ;
        }
    }
    return 0;
}
int versionReportRsp(void* pContext,JHandle handle,const VersionReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("versionReportRsp err code:[%d]\n",errcode);
    }

    printf("versionReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    printf("OK\n");
    return 0;
}
int versionReportReq(JHandle handle,DEV*pDev,char* app_ver)
{
    VersionReportReq req ;
    req.seq_no =  0;
    req.app_ver= app_ver;

    JiotResult JRet  =  jiotVersionReportReq(handle,&req);

    if(JRet.errCode == 0)
    {
        printf("message seqNo:[%lld]\n", JRet.seqNo );
    }
	return JRet.errCode;
}
// ota entrance
int httpOTA(httpOTAParam* param)
{
	printf("httpOTA url[%s] taskId[%ld]\n", param->url, param->taskId);
	// init
    httpOTACallbackParam cbPara;
	cbPara.step = OTA_STEP_INIT;
	cbPara.desc = "OTA INIT";
	cbPara.taskId = param->taskId;
	param->callback(&cbPara);

    otaHttpClient* httpClient = otaHttpGet(param->url);
    if (httpClient == NULL)
    {
        printf("httpOTA otaHttpGet failed\n");
        return -1;
    }
    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    if(otaFirmwareSaveInit(param->version))
    {
    	printf("httpOTA otaFirmwareSaveInit failed\n");
    	return -1;
    }

    // downloading
	cbPara.step = OTA_STEP_HTTP_DOWNLOAD_START;
	cbPara.desc = "DOWNLOAD START";
	param->callback(&cbPara);

    int totalLen = 0;
    do
    {
        int len = otaHttpFetch(httpClient, 3000);
        if (len == 0)
        {
            printf("httpOTA connection closed,all data received\n");
            break;
        }
        if (len < 0)
        {
        	cbPara.step = OTA_STEP_HTTP_DOWNLOAD_FAILED;
        	cbPara.desc = "DOWNLOAD FAILED";
        	param->callback(&cbPara);
            printf("httpOTA data read error\n");
            return -1;
        }
        if (len > 0)
        {
            totalLen += len;
            otaFirmwareSaving(httpClient->data.response_buf, len);
            MD5_Update(&md5_ctx, (unsigned char const *)httpClient->data.response_buf, (unsigned)len);

        	cbPara.step = OTA_STEP_HTTP_DOWNLOADING;
        	cbPara.desc = "DOWNLOADING";
        	cbPara.downloadPercent = totalLen/(param->firmwareSize/100);
        	param->callback(&cbPara);
        }
    } while(otaHttpNeedFetchMore(httpClient));

    // download finish
	cbPara.step = OTA_STEP_HTTP_DOWNLOAD_FINASHED;
	cbPara.desc = "DOWNLOAD FINASHED";
	param->callback(&cbPara);
    otaHttpFinish(httpClient);
    otaFirmwareFinish();
    printf("httpOTA Total binary data length writen: %d\n", totalLen);

    // firmware check
	cbPara.step = OTA_STEP_FIRMWARE_CHECK;
	cbPara.desc = "FIRMWARE CHECK";
	param->callback(&cbPara);
    unsigned char digest[16];
    MD5_Final(digest, &md5_ctx);
    if(0 != otaMD5Check(digest, sizeof(digest), param->checksum))
    {
    	cbPara.step = OTA_STEP_FIRMWARE_CHECK_FAILED;
    	cbPara.desc = "CHECK FAILED";
    	param->callback(&cbPara);
    	printf("httpOTA firmware checked failed\n");
    	return -1;
    }

	cbPara.step = OTA_STEP_SYSTEM_REBOOT;
	cbPara.desc = "SYSTEM REBOOT";
	param->callback(&cbPara);

    // system reboot
	printf("httpOTA otaSystemReboot\n");
    otaSystemReboot();
    return 0;
}

int main(int argc,char *argv[])
{
    printf("start ota demo!\n");

    jiotSetLogLevel(ERROR_LOG_LEVL);

	int  ret = 0 ;
    DeviceInfo devinfo ;
    if (argc != 4 )
    {
        printf("err input ! use [demo ProductKey DeviceName DeviceSecret ] \n");
        return 0;
    }
    else
    {
        strcpy(devinfo.szProductKey,argv[1]);
        strcpy(devinfo.szDeviceName,argv[2]);
        strcpy(devinfo.szDeviceSecret,argv[3]);
    }

    DEV Dev ;
    Dev.seq_no = 1 ;
    Dev.version = 1 ;
    memset((void*)Dev.app_ver,0,32);
    memset((void*)Dev.msg,0,1024);

    JClientMessageCallback cb;
    cb._cbVersionReportRsp  =   versionReportRsp;
	cb._cbOtaUpgradeInformReq     =   otaUpgradeReq;
	cb._cbOtaStatusReportRsp      =   otaReportRsp;

    JClientHandleCallback handleCb;
    handleCb._cbConnectedHandle     = connectedHandle;
    handleCb._cbConnectFailHandle   = connectFailHandle;
    handleCb._cbDisconnectHandle    = disconnectHandle;
    handleCb._cbSubscribeFailHandle = subscribeFailHandle;
    handleCb._cbPublishFailHandle   = NULL ;
    handleCb._cbMessageTimeoutHandle= messageTimeoutHandle;

    int nProperty_size = 1;
    Dev.pProperty = (Property*)malloc(sizeof(Property)*nProperty_size);
    Dev.pEvent = (Event*)malloc(sizeof(Event));

    JHandle handle = jiotInit();
    if (handle ==NULL)
    {
        printf("jClient init err \n");
        return 0;
    }

    jiotRegister(handle,&Dev,&cb,&handleCb);

    ret = jiotConn(handle,&devinfo);
    if(ret != 0)
    {
        printf("connect err,errno:%d\n",ret);
        return 0;
    }

    printf("Connection, please later \n");


    check(handle);
    gClienthandle = handle;
    /* 版本上报 */
    {
		printf("version report:\n");
		strncpy(Dev.app_ver,CURRENT_FIRMWARE_VERSION,32); //notice: if version not match ota version ota failed

		ret = versionReportReq(handle, &Dev, Dev.app_ver);
		if(ret == 0)
		{
		  printf("send version report \n");
		}
		else
		{
		  printf("send version report err,code:[%d]\n",ret);
		}
    }

    int i = 0;
	for(; i < 200; ++i)
	{
		printf("firmwire version[%s], wait for ota command. count[%d]\n", Dev.app_ver, i);
		if(gHttpOTAProcessing)
		{
			if(httpOTA(&gHttpOTAParam))
			{
				printf("ota failed\n");
			}
			// ota finished, exit
			gHttpOTAProcessing = 0;
			if(gExit)
			{
				break;
			}
		}

		sleep(2);
	}

    jiotDisConn(handle);
    jiotUnRegister(handle);
    jiotRelease(&handle);
    free(Dev.pProperty);
    free(Dev.pEvent);

    printf("end of ota demo!\n");
    return 0;
}





