#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"

#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "jiot_client.h"

/*
configs for demo:
*/
#define WIFI_SSID			"yourSSID"
#define WIFI_PASSWORD		"yourPASSWORD"
#define JIOT_PRODUCTKEY		"yourPRODUCTKEY"
#define JIOT_DEVICENAME		"yourDEVICENAME"
#define JIOT_DEVICESECRET	"yourDEVICESECRET"

/*
the levels for jiot-sdk:
	ESP_LOG_NONE,
	ESP_LOG_ERROR,
	ESP_LOG_WARN,
	ESP_LOG_INFO,
	ESP_LOG_DEBUG,
*/

static const char *TAG = "demo";

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

#define D_DECSEPER 		1000000000ULL			// 10^9
#define D_DECSEPER2 	1000000000000000000ULL	// 10^18
int lltoa(char* buf, long long num) //need a buf with 21 bytes
{
	char symble[2] = {0};
	if(num < 0)
	{
		symble[0] = '-';
		num = llabs(num);
	}

	int ret = -1;
	if(num < D_DECSEPER)
	{
		ret = sprintf(buf, "%s%d", symble, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	else if(num >= D_DECSEPER && num < D_DECSEPER2)
	{
		ret = sprintf(buf, "%s%u%09u", symble, (unsigned int)(num/D_DECSEPER) &0xFFFFFFFF, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	else if(num >= D_DECSEPER2)
	{
		ret = sprintf(buf, "%s%u%09u%09u", symble, (unsigned int)(num/D_DECSEPER2) &0xFFFFFFFF, (unsigned int)((num/D_DECSEPER)%D_DECSEPER) &0xFFFFFFFF, (unsigned int)(num%D_DECSEPER) &0xFFFFFFFF);
	}
	return ret;
}

#define PRINT_SEQNO(seqNo)\
    do\
    {\
    	char buf[21] = {0};\
    	lltoa(buf, seqNo);\
        printf("message seqNo:[%s]\n", buf);\
    }\
    while(0)

int propertyReportReq(JHandle handle, DEV* pDev,Property * property,int size)
{
    PropertyReportReq req ;
    req.seq_no =  0;
    req.version = pDev->version;
    req.property_size= size;

    Property * tempProperty =  (Property*)malloc(sizeof(Property)*size);
    memset((void*)tempProperty,0,sizeof(Property)*size);
    int i =0 ;
    for (i = 0; i < size; ++i)
    {
        tempProperty[i].name = property[i].name;
        tempProperty[i].value = property[i].value;
        tempProperty[i].time = property[i].time;
    }

    req.pProperty = tempProperty;

    JiotResult JRet = jiotPropertyReportReq(handle,&req);

	if (tempProperty != NULL)
    {
        free(tempProperty);
    }
    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}

int eventReportReq(JHandle handle,DEV* pDev,Event *event )
{
    EventReportReq req ;
    req.seq_no =  0;

    Event tempEvent ;
    tempEvent.name = event->name;
    tempEvent.content = event->content;
    tempEvent.time = event->time;

    req.pEvent = &tempEvent;

    JiotResult JRet = jiotEventReportReq(handle,&req);

    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}

int versionReportReq(JHandle handle,DEV*pDev,char* app_ver)
{
    VersionReportReq req ;
    req.seq_no =  0;
    req.app_ver= app_ver;

    JiotResult JRet  =  jiotVersionReportReq(handle,&req);

    if(JRet.errCode == 0)
    {
    	PRINT_SEQNO(JRet.seqNo);
    }
	return JRet.errCode;
}


int propertyReportRsp(void* pContext,JHandle handle,const PropertyReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("propertyReportRsp err code:[%d]\n",errcode);
        return 0;
    }

    printf("propertyReportRsp:\n");
    DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        if(Rsp->version != 0)
        {
            pDev->version = Rsp->version+1;
            pDev->oldVersion = pDev->version;
        }
        printf("ERR: code [%d]\n", Rsp->code );
    }
    else
    {
        pDev->oldVersion = pDev->version;
        printf("OK\n");
    }

    return 0;
}

int eventReportRsp(void* pContext,JHandle handle,const EventReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        printf("eventReportRsp err code:[%d]\n",errcode);
    }

    printf("eventReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    printf("OK\n");
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

int propertySetReq(void* pContext,JHandle handle,PropertySetReq *Req,int errcode)
{
    if (errcode != 0)
    {
        printf("propertySetReq msg err code:[%d]\n",errcode);
    }

    printf("propertySetReq:\n");
    DEV* pDev = (DEV*)pContext;
    pDev->desired_version = Req->version;

    Property * pProperty = malloc(sizeof(Property)*Req->property_size);
    int i =0;
    for (i=0; i < Req->property_size; ++i)
    {
        printf("property.name:[%s]\nproperty.value:[%s]\n",  Req->pProperty[i].name,Req->pProperty[i].value);

        struct tm *tblock;
        time_t timer = Req->pProperty[i].time/1000;
        tblock = localtime(&timer);
        printf("property.time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);

        pProperty->name = Req->pProperty[i].name;
        pProperty->value = Req->pProperty[i].value;
        pProperty->time = Req->pProperty[i].time;
    }

    if (pDev->version == pDev->oldVersion)
    {
        pDev->version ++ ;
    }

    free(pProperty);
    return 0;

}

int msgDeliverReq(void* pContext,JHandle handle,MsgDeliverReq *Req,int errcode)
{
    if (errcode != 0)
    {
        printf("msgDeliverReq msg err code:[%d]\n",errcode);
    }

    printf("msgDeliverReq:\n");
	//pContext 之前注册的上下文信息
    //DEV* pDev = (DEV*)pContext;

    printf("message:[%s]\n", Req->message);
    struct tm *tblock;
    time_t timer = Req->time;
    tblock = localtime(&timer);

    printf("time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);


    return 0;

}

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
    printf("\nJclient message time out ");
    PRINT_SEQNO(seqNo);
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


int demo(const char* productKey, const char* deviceName, const char* deviceSecret)
{
    printf("\nstart demo! @%ld\n", time(NULL));
	int  ret = 0 ;

	jiotSetLogLevel(DEBUG_LOG_LEVL);
    DeviceInfo devinfo ;
    {
        strcpy(devinfo.szProductKey, productKey);
        strcpy(devinfo.szDeviceName, deviceName);
        strcpy(devinfo.szDeviceSecret, deviceSecret);
    }


    DEV Dev ;
	Dev.seq_no = 1 ;
	Dev.version = 1 ;
	memset((void*)Dev.app_ver,0,32);
	memset((void*)Dev.msg,0,1024);

	JClientMessageCallback cb;
	cb._cbPropertyReportRsp =   propertyReportRsp ;
	cb._cbEventReportRsp    =   eventReportRsp;
	cb._cbVersionReportRsp  =   versionReportRsp;
	cb._cbPropertySetReq    =   propertySetReq;
	cb._cbMsgDeliverReq     =   msgDeliverReq;


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

	for(int i = 0; i < 1; ++i)
	{
		/* 属性上报*/
		{
			printf("property report:\n");
			Dev.pProperty->name = "switch";
			Dev.pProperty->value="1";

			Dev.pProperty->time = (long long )time(NULL)*1000; //毫秒
			ret = propertyReportReq(handle,&Dev,Dev.pProperty,1);
			if(ret == 0)
			{
				printf("send property report\n");
			}
			else
			{
				printf("send property report err,code:[%d]\n",ret);
			}
		}

		/* 事件上报*/
		{
			printf("event report:\n");
			Dev.pEvent->name = "event";
			Dev.pEvent->content = "This content of event!";
			Dev.pEvent->time = (long long)time(NULL);
			int ret = eventReportReq(handle,&Dev,Dev.pEvent);
			if(ret == 0)
			{
				printf("send event report \n");
			}
			else
			{
				printf("send event report err,code:[%d]\n",ret);
			}
		}

        /* 版本上报 */
		{
          printf("version report:\n");
          strncpy(Dev.app_ver,"dengdeng1.1.0",32);

          ret = versionReportReq(handle,&Dev,Dev.app_ver);
          if(ret == 0)
          {
              printf("send version report \n");
          }
          else
          {
              printf("send version report err,code:[%d]\n",ret);
          }
		}

		sleep(10);
	}

	// waiting to accept message
	for(int i = 0; i < 5; ++i)
	{
		printf("waiting to accept message \n");
		sleep(10);
	}


	jiotDisConn(handle);
    printf("jiotDisConn \n");

	jiotUnRegister(handle);
    printf("jiotUnRegister  \n");

	jiotRelease(&handle);
    printf("jiotRelease \n");

	free(Dev.pProperty);
	free(Dev.pEvent);


	printf("demo exit \n");
    return 0;
}


/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static void obtain_time(void)
{
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                        false, true, portMAX_DELAY);
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    default:
        break;
    }

    return ESP_OK;
}
static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "before esp_get_free_heap_size [%u]", esp_get_free_heap_size());
    ESP_LOGI(TAG, "before esp_get_free_heap_size [%u]", esp_get_minimum_free_heap_size());
    // wifi
    initialise_wifi();
    ESP_LOGI(TAG, "wait wifi connect...");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    // ntp
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
    }
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();

    // run demo
    demo(JIOT_PRODUCTKEY, JIOT_DEVICENAME, JIOT_DEVICESECRET);


}
