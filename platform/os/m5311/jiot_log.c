/*
 * jiot_log.c
 *
 *  Created on: 2019年4月29日
 *      Author: guowenhe
 */

#include "jiot_log.h"
#include "hal_uart.h"
#include "semphr.h"

int g_logLevl = NO_LOG_LEVL ;

hal_uart_config_t g_logUartConfig;
SemaphoreHandle_t  g_uartPrintfMutex = NULL;
void uart_open()
{
	g_logUartConfig.baudrate = HAL_UART_BAUDRATE_115200;
	g_logUartConfig.word_length = HAL_UART_WORD_LENGTH_8;
	g_logUartConfig.stop_bit = HAL_UART_STOP_BIT_1;
	g_logUartConfig.parity = HAL_UART_PARITY_NONE;

	hal_uart_status_t status = hal_uart_init(UART_PRINTF_PORT, &g_logUartConfig);
	if (HAL_UART_STATUS_OK == status)
	{
//		opencpu_printf("uart_open succeed\n");
	}
	else
	{
//		opencpu_printf("uart_open failed\n");
	}
}

void uart_send(const uint8_t *data, uint32_t size)
{
	uint32_t ret = hal_uart_send_polling(UART_PRINTF_PORT, data, size);

	if(ret == size)
	{
//		opencpu_printf("send all bytes[%d]\n", ret);
	}
	else
	{
//		opencpu_printf("send part total[%d], bytes[%d]\n", ret, size);
	}
}

void uart_printf(const char *str, ...)
{
    if ((str == NULL) || (strlen(str) == 0))
    {
        return;
    }
    va_list args;
    va_start (args, str);
    unsigned char buffer[UART_PRINTF_MAX_LEN];
    int str_len = (unsigned int)vsnprintf ((char*)buffer, UART_PRINTF_MAX_LEN,  str, args);
    va_end (args);

    buffer[UART_PRINTF_MAX_LEN - 1] = 0;
    if(str_len > UART_PRINTF_MAX_LEN)
    {
    	str_len = UART_PRINTF_MAX_LEN;
    }

	if(NULL == g_uartPrintfMutex)
	{
//		opencpu_printf("g_uartPrintfMutex created error\n");
		return;
	}
    xSemaphoreTake(g_uartPrintfMutex, portMAX_DELAY );
    uart_send(buffer, str_len);
    xSemaphoreGive(g_uartPrintfMutex);

}
void uart_printf_init()
{
	g_uartPrintfMutex = xSemaphoreCreateMutex();
	if(NULL == g_uartPrintfMutex)
	{
//		opencpu_printf("g_uartPrintfMutex created error\n");
		return;
	}
	uart_open();
}



