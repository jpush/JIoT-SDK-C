#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "jiot_client.h"
#include "m5311_opencpu.h"

/*
configs for demo:
*/


#define JIOT_PRODUCTKEY		"yourPRODUCTKEY"
#define JIOT_DEVICENAME		"yourDEVICENAME"
#define JIOT_DEVICESECRET	"yourDEVICESECRET"

int gIsNtpOK = 0;


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
        uart_printf("message seqNo:[%s]\n", buf);\
    }\
    while(0)



void opencpu_fota_progress_cb(int current,int total)
{

}
void opencpu_fota_event_cb(int event,int state)
{


}
unsigned long opencpu_fota_version_cb()
{
	return "NULL";
}


void opencpu_stack_overflow_hook(xTaskHandle *pxTask, signed portCHAR * pcTaskName)
{

}
void vApplicationTickHook( void )
{

}
void opencpu_task_idle_hook(void)
{

}

/********************************************************************************/
//此函数为opencpu产线模式相关的回调函数，返回1则版本下载到模组后，以AT命令方式启动，需要先执行AT+ATCLOSE命令，之后才会以opencpu方式启动。
//返回0则以opencpu方式启动
//请务必联系技术支持后再确定返回值
//请客户务必保留此函数中所有代码，仅根据需求调整log输出的串口号和返回值。否则可能造成功能紊乱

#define GKI_LOG_NAME "emmi"
#define HSL_LOG_NAME "uls"
#define USER_GKI_LOG_PORT SERIAL_PORT_DEV_USB_COM1    //请在这里修改用户GKI log的输出串口，默认为USB COM2
#define USER_HSL_LOG_PORT SERIAL_PORT_DEV_USB_COM2  //请在这里修改用户HSL log的输出串口，默认为USB COM1
int get_factory_mode(void)
{

	//以下代码是判断模组log口的输出串口号，确保生效的设置和用户的设置一致，防止log口设置错误而干扰用户
	serial_port_dev_t gki_port = -1;
	serial_port_dev_t hsl_port = -1;

	opencpu_read_port_config(GKI_LOG_NAME,&gki_port);
	opencpu_read_port_config(HSL_LOG_NAME,&hsl_port);

	if( (gki_port != USER_GKI_LOG_PORT) || (hsl_port != USER_HSL_LOG_PORT)) //根据读的结果决定是否写
	{
		opencpu_write_port_config(GKI_LOG_NAME,USER_GKI_LOG_PORT);
		opencpu_write_port_config(HSL_LOG_NAME,USER_HSL_LOG_PORT);
		opencpu_reboot();//因为设置是重启生效，所以设置完必须有reboot函数
	}


	return 1;
}

int gWeakUpTimes = 0;
void opencpu_wakeup_callback()
{
	//注意此处不能用 uart_printf打印日志，会导致重启，可以用下面变量确认是否发生wakeup
	gWeakUpTimes++;
}


//测试获取ICCID
void test_get_iccid()
{
	unsigned char buf[30];
	int i = 0;
	memset(buf,0,30);
	while(opencpu_iccid(buf)!= 0)
	{
		i++;
		vTaskDelay(10);
		if(i>20)
		{
			uart_printf("iccid timeout\n");
			return;
		}
	}
	uart_printf("ICCID:%s\n",buf);
}




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
        uart_printf("propertyReportRsp err code:[%d]\n",errcode);
        return 0;
    }

    uart_printf("propertyReportRsp:\n");
    DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        if(Rsp->version != 0)
        {
            pDev->version = Rsp->version+1;
            pDev->oldVersion = pDev->version;
        }
        uart_printf("ERR: code [%d]\n", Rsp->code );
    }
    else
    {
        pDev->oldVersion = pDev->version;
        uart_printf("OK\n");
    }

    return 0;
}

int eventReportRsp(void* pContext,JHandle handle,const EventReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        uart_printf("eventReportRsp err code:[%d]\n",errcode);
    }

    uart_printf("eventReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        uart_printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    uart_printf("OK\n");
    return 0;
}



int versionReportRsp(void* pContext,JHandle handle,const VersionReportRsp * Rsp,int errcode)
{
    if (errcode != 0)
    {
        uart_printf("versionReportRsp err code:[%d]\n",errcode);
    }

    uart_printf("versionReportRsp:\n");
    //DEV* pDev = (DEV*)pContext;
    if (Rsp->code != 0)
    {
        uart_printf("ERR: code [%d]\n", Rsp->code );
        return 0;
    }

    uart_printf("OK\n");
    return 0;
}

int propertySetReq(void* pContext,JHandle handle,PropertySetReq *Req,int errcode)
{
    if (errcode != 0)
    {
        uart_printf("propertySetReq msg err code:[%d]\n",errcode);
    }

    uart_printf("propertySetReq:\n");
    DEV* pDev = (DEV*)pContext;
    pDev->desired_version = Req->version;

    Property * pProperty = malloc(sizeof(Property)*Req->property_size);
    int i =0;
    for (i=0; i < Req->property_size; ++i)
    {
        uart_printf("property.name:[%s]\nproperty.value:[%s]\n",  Req->pProperty[i].name,Req->pProperty[i].value);

        struct tm *tblock;
        time_t timer = Req->pProperty[i].time/1000;
        tblock = localtime(&timer);
        uart_printf("property.time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);

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
        uart_printf("msgDeliverReq msg err code:[%d]\n",errcode);
    }

    uart_printf("msgDeliverReq:\n");
    DEV* pDev = (DEV*)pContext;

    uart_printf("message:[%s]\n", Req->message);
    struct tm *tblock;
    time_t timer = Req->time;
    tblock = localtime(&timer);

    uart_printf("time:[%d/%d/%d %d:%d:%d]\n", tblock->tm_year+1900,tblock->tm_mon+1,tblock->tm_mday,tblock->tm_hour,tblock->tm_min,tblock->tm_sec);


    return 0;

}

int connectedHandle(void* pContext)
{
    uart_printf("\nJclient connected \n");
    return 0;
}

int connectFailHandle(void* pContext,int errCode)
{
    uart_printf("\nJclient connect fail . Code:[%d] \n",errCode);
    return 0;
}

int disconnectHandle(void* pContext,int errCode)
{
    uart_printf("\nJclient disconnect . Code:[%d] \n",errCode);
    return 0;
}

int subscribeFailHandle(void* pContext,char * TopicFilter)
{
    uart_printf("\nJclient subscribe topic fail .Topic[%s] \n",TopicFilter);
    return 0;
}

int messageTimeoutHandle(void* pContext,long long seqNo)
{
    uart_printf("\nJclient message time out ");
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
            uart_printf(".");
            fflush(stdout);
            vTaskDelay(100);
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
	jiotSetLogLevel(ERROR_LOG_LEVL);
	uart_printf("\nstart demo!  freeheap[%d]\n",  xPortGetFreeHeapSize());

	int  ret = 0 ;

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
		uart_printf("jClient init err \n");
		return 0;
	}

	jiotRegister(handle,&Dev,&cb,&handleCb);

	ret = jiotConn(handle,&devinfo);
	if(ret != 0)
	{
		uart_printf("connect err,errno:%d\n",ret);
		return 0;
	}

	uart_printf("Connection, please later \n");

	check(handle);

	for(int i = 0; i < 1; ++i)
	{
		/* 属性上报*/
		{
			uart_printf("property report:\n");
			Dev.pProperty->name = "switch";
			Dev.pProperty->value="1";

			Dev.pProperty->time = (long long )time(NULL)*1000; //毫秒
			ret = propertyReportReq(handle,&Dev,Dev.pProperty,1);
			if(ret == 0)
			{
				uart_printf("send property report\n");
			}
			else
			{
				uart_printf("send property report err,code:[%d]\n",ret);
			}
		}

		/* 事件上报*/
		{
			uart_printf("event report:\n");
			Dev.pEvent->name = "event";
			Dev.pEvent->content = "This content of event 112233!";
			Dev.pEvent->time = (long long)time(NULL);
			int ret = eventReportReq(handle,&Dev,Dev.pEvent);
			if(ret == 0)
			{
				uart_printf("send event report \n");
			}
			else
			{
				uart_printf("send event report err,code:[%d]\n",ret);
			}
		}

        /* 版本上报 */
		{
          uart_printf("version report:\n");
          strncpy(Dev.app_ver,"dengdeng1.1.0",32);

          ret = versionReportReq(handle,&Dev,Dev.app_ver);
          if(ret == 0)
          {
              uart_printf("send version report \n");
          }
          else
          {
              uart_printf("send version report err,code:[%d]\n",ret);
          }
		}

		vTaskDelay(1000);
	}

	// waiting to accept message
	for(int i = 0; i < 10; ++i)
	{
		uart_printf("wait for message, times[%d]\n", i);
		vTaskDelay(1000); // 10s
	}


	jiotDisConn(handle);
    uart_printf("after jiotDisConn \n");

	jiotUnRegister(handle);
    uart_printf("after jiotUnRegister  \n");
	jiotRelease(&handle);
    uart_printf("after jiotRelease \n");

	free(Dev.pProperty);
	free(Dev.pEvent);

	uart_printf("\ndemo exit! freeheap[%d]\n",  xPortGetFreeHeapSize());
    return 0;
}

void ntp_cb(unsigned char * info)
{
	uart_printf("ntp_cb[%s] \n", info);
	gIsNtpOK = 1;
}

void jiot_demo_task()
{
    hal_spi_master_config_t l_config;
	hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;
	uart_printf_init();

	uart_printf("\n\n");
	uart_printf("run mode:%d\n",get_run_mode());
	uart_printf("M5311 opencpu ready!!\n");
	uart_printf("waiting for network...\n");
    opencpu_lock_light_sleep();
	test_get_iccid();
	uart_printf("network registering...\n");
	while(opencpu_cgact()!=1)
	{
		vTaskDelay(10);
	}
	uart_printf("network register success\n");
	uart_printf("network ready!!\n");

	// close psm
	ril_power_saving_mode_setting_req_t psmReq;
	psmReq.mode=0;
	psmReq.req_prdc_rau=NULL;
	psmReq.req_gprs_rdy_tmr=NULL;
	psmReq.req_prdc_tau="00101011";
	psmReq.req_act_time="00100100";
	int psmReqRet = opencpu_set_psmparam(&psmReq);
	uart_printf("opencpu_set_psmparam retPsmReq[%d] \n",psmReqRet);
	ril_power_saving_mode_setting_rsp_t respReqRet;
	int retPsmResp = opencpu_get_psmparam(&respReqRet);
	uart_printf("opencpu_get_psmparam respReqRet[%d] mode[%d] \n",retPsmResp, respReqRet.mode);

	// close edrx
	unsigned char edrx_value[256]= {0};
	int edrxRet = opencpu_set_edrx(0, 5,edrx_value);
	uart_printf(" set edrxRet[%d] edrx_value[%s] \n",edrxRet, edrx_value);
	int act_type = 5;
	int edrxReadRet = opencpu_read_edrx(&act_type, edrx_value);
	uart_printf("edrxReadRet[%d] edrx_value[%s] \n",edrxReadRet, edrx_value);

	// ntp
	uart_printf("ntp start\n");
	for(int i = 0; i < 5; ++i)
	{
		int ret = opencpu_cmntp("pool.ntp.org", 123, 1, 120, ntp_cb);
		uart_printf("ntp times[%d], ret[%d]\n", i + 1, ret);
		if(ret == 0)
		{
			break;
		}
		vTaskDelay(100);
	}
	unsigned char time_string[50];
	memset(time_string,0,50);
	uart_printf("ntp-while for wait");
	while(!gIsNtpOK)
	{
		uart_printf(".");
		opencpu_rtc_get_time(time_string);
		if(time_string[0] != 0)
		{
			break;
		}
		vTaskDelay(100);
	}

    // run demo
    demo(JIOT_PRODUCTKEY, JIOT_DEVICENAME, JIOT_DEVICESECRET);
    vTaskDelete(NULL);
}


void test_opencpu_start(void)
{
    xTaskCreate(jiot_demo_task,"jiot_demo_task",3072,NULL,TASK_PRIORITY_NORMAL,NULL);
}
