/*
 * user_ota.c
 *
 *  Created on: Jun 28, 2019
 *      Author: guowenhe
 */

#include "user_ota.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include <unistd.h>
#include <string.h>
#include "esp_ota_ops.h"
#include "esp_task_wdt.h"
#include "sodium.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "rom/md5_hash.h"

int gFirmwareSize = 0;
int gFirmwareExpectedSize = 0;
long gHttpOTATaskId = 0;
httpOTACallback gHttpOTACallback = NULL;
#define HTTP_OTA_BUF_SIZE 256


// importent step, need retry
// retry #1  after 600 second
// retry #2  after 3600 second
#define OTA_HTTP_CALLBACK(cbPara, cbStep, cbDesc) \
	do \
	{ \
		cbPara.step = cbStep; \
		cbPara.desc = cbDesc; \
		cbPara.taskId = gHttpOTATaskId; \
		if(0 != gHttpOTACallback(&cbPara)) \
		{ \
			sleep(600); \
			if(0 != gHttpOTACallback(&cbPara)) \
			{ \
				sleep(3600); \
				gHttpOTACallback(&cbPara); \
			} \
		} \
	} \
	while(0)

#define OTA_HTTP_CALLBACK_PERCENT(cbPara, cbStep, cbDesc, cbPercent) \
	do \
	{ \
		cbPara.step = cbStep; \
		cbPara.desc = cbDesc; \
		cbPara.taskId = gHttpOTATaskId; \
		cbPara.downloadPercent = cbPercent; \
		gHttpOTACallback(&cbPara); \
	} \
	while(0)

static void httpCleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

esp_err_t httpEventHandler(esp_http_client_event_t *evt)
{
	httpOTACallbackParam cbPara;
    switch(evt->event_id)
    {
        case HTTP_EVENT_ON_CONNECTED:
        	OTA_HTTP_CALLBACK_PERCENT(cbPara, OTA_STEP_HTTP_DOWNLOAD_START, "DOWNLOAD_START", 0);
        	break;
        case HTTP_EVENT_ON_DATA:
        	gFirmwareSize += evt->data_len;
        	cbPara.downloadPercent = (gFirmwareSize * 100) / gFirmwareExpectedSize;
//        	printf("FirmwareSize[%d] gFirmwareExpectedSize[%d]\n", gFirmwareSize, gFirmwareExpectedSize);
        	OTA_HTTP_CALLBACK_PERCENT(cbPara, OTA_STEP_HTTP_DOWNLOADING, "OTA_STEP_HTTP_DOWNLOADING", cbPara.downloadPercent);
            break;
        default:
        	break;
    }
    return ESP_OK;
}

int md5Check(httpOTACallbackParam* cbPara, unsigned char* digest, size_t digestLen, const char* checksum)
{
 	cbPara->step = OTA_STEP_FIRMWARE_CHECK;
 	cbPara->desc = "FIRMWARE_CHECK";
 	cbPara->taskId = gHttpOTATaskId;
 	gHttpOTACallback(cbPara);

    size_t checksumLen = strlen(checksum);
    unsigned char digest2[16] = {0};
    sodium_hex2bin(digest2, sizeof(digest2), checksum, checksumLen, NULL, NULL, NULL);

	char digestHexStr[33]={0};
	sodium_bin2hex(digestHexStr, sizeof(digestHexStr), digest, digestLen);
	printf("md5Check firmware checked digest[%s] checksum[%s]\n", digestHexStr, checksum);

 	if(checksumLen != (digestLen * 2)
 			|| memcmp(digest, digest2, digestLen) != 0)
 	{
 		return ESP_FAIL;
 	}
    printf("md5Check pass\n");
    return 0;
}
extern int gSystemReboot;
void httpOTAReboot(httpOTACallbackParam* cbPara)
{
	cbPara->step = OTA_STEP_SYSTEM_REBOOT;
	cbPara->desc = "SYSTEM_REBOOT";
	cbPara->taskId = gHttpOTATaskId;
	gHttpOTACallback(cbPara);
	gSystemReboot = 1;
}

// copy form esp_https_ota.c, delete some limit and add md5 check
esp_err_t httpOTA(httpOTAParam* param)
{
	printf("httpOTA url[%s] taskId[%ld]\n", param->url, param->taskId);
	gHttpOTACallback = param->callback;
	gHttpOTATaskId = param->taskId;
    httpOTACallbackParam cbPara;

    OTA_HTTP_CALLBACK(cbPara, OTA_STEP_INIT, "INIT");
    esp_http_client_config_t config =	{
											.url = param->url,
											.timeout_ms = 300000, // 5 minute
											.auth_type = HTTP_AUTH_TYPE_NONE,
											.cert_pem = NULL,
											.event_handler = httpEventHandler,
										};
    gFirmwareExpectedSize = param->firmwareSize;
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL)
    {
        printf("Failed to initialise HTTP connection\n");
    	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "HTTP_INIT_FAILED");
        return ESP_FAIL;
    }
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK)
    {
        esp_http_client_cleanup(client);
        printf("Failed to open HTTP connection: %d\n", err);
    	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "HTTP_OPEN_FAILED");
        return err;
    }
    esp_http_client_fetch_headers(client);
    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL)
    {
        printf("Passive OTA partition not found\n");
        httpCleanup(client);
    	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "GET_NEXT_PARTITION_FAILED");
        return ESP_FAIL;
    }
    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        printf("esp_ota_begin failed, error=%d\n", err);
        httpCleanup(client);
    	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "ESP_OTA_BEGIN_FAILED");
        return err;
    }

    printf("Please Wait. This may take time\n");
    esp_err_t ota_write_err = ESP_OK;
    char *upgrade_data_buf = (char *)malloc(HTTP_OTA_BUF_SIZE);
    if (!upgrade_data_buf)
    {
        printf("Couldn't allocate memory to upgrade data buffer\n");
        OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "ESP_OTA_ALLOCATE_MEM_FAILED");
        return ESP_ERR_NO_MEM;
    }
    struct MD5Context md5_ctx;
    MD5Init(&md5_ctx);
    int binary_file_len = 0;
    gFirmwareSize = 0;
    while(1)
    {
        int data_read = esp_http_client_read(client, upgrade_data_buf, HTTP_OTA_BUF_SIZE);
        if (data_read == 0)
        {
            printf("Connection closed,all data received\n");
            break;
        }
        if (data_read < 0)
        {
            printf("Error: data read error\n");
            OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "ESP_OTA_DATA_READ_FAILED");
            break;
        }
        if (data_read > 0)
        {
        	MD5Update(&md5_ctx, (unsigned char const *)upgrade_data_buf, (unsigned)data_read);
        	ota_write_err = esp_ota_write( update_handle, (const void *)upgrade_data_buf, data_read);
            if (ota_write_err != ESP_OK)
            {
                break;
            }
            binary_file_len += data_read;
        }
    }
    free(upgrade_data_buf);
    httpCleanup(client);
    esp_err_t ota_end_err = esp_ota_end(update_handle);
    if (ota_write_err != ESP_OK)
    {
        printf("Error: esp_ota_write failed! err=0x%d\n", err);
        OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "ESP_OTA_ESP_OTA_WRITE_FAILED");
        return ota_write_err;
    }
    else if (ota_end_err != ESP_OK)
    {
        printf("Error: esp_ota_end failed! err=0x%d. Image is invalid\n", ota_end_err);
        OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FAILED, "ESP_OTA_ESP_OTA_FAILED");
        return ota_end_err;
    }

 	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_HTTP_DOWNLOAD_FINASHED, "DOWNLOAD_FINASHED");

    unsigned char digest[16];
    MD5Final(digest, &md5_ctx);
    if(0 != md5Check(&cbPara, digest, sizeof(digest), param->checksum))
    {
    	printf("firmware checked failed\n");
    	OTA_HTTP_CALLBACK(cbPara, OTA_STEP_FIRMWARE_CHECK_FAILED, "OTA_STEP_FIRMWARE_CHECK_FAILED");
    	return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        printf("esp_ota_set_boot_partition failed! err=0x%d", err);
        OTA_HTTP_CALLBACK(cbPara, OTA_STEP_FAILED, "OTA_STEP_SET_BOOST_PARTITION_FAILED");
        return err;
    }

    httpOTAReboot(&cbPara);

    return ESP_OK;
}





