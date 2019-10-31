/* Copyright (C) 2012 mbed.org, MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Copyright (c) 2014-2016 Alibaba Group. All rights reserved.
 * License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef JIOT_HTTPCLIENT_H
#define JIOT_HTTPCLIENT_H

#include "jiot_std.h"
#include "jiot_err.h"
#include "jiot_log.h"
#include "jiot_common.h"


#ifdef __cplusplus
extern "C"
{
#endif


#define  JIOT_ERR_HTTP_RETRIEVE_MORE_DATA            -18001               //请求响应后还有未取完的数据
#define  JIOT_ERR_HTTP_CONN                          -18002               //连接失败
#define  JIOT_ERR_HTTP_PARSE                         -18003               //URL解析错误
#define  JIOT_ERR_HTTP_UNRESOLVED_DNS                -18004               //无法解析的Hostname
#define  JIOT_ERR_HTTP_PRTCL                         -18005               //协议错误
#define  JIOT_ERR_HTTP_ERROR                         -18006               //HTTP位置错误 
#define  JIOT_ERR_HTTP_CLOSED                        -18007               //远程Host关闭连接
#define  JIOT_ERR_HTTP_BREAK                         -18008               //连接中断
#define  JIOT_ERR_HTTP_HEADER_TOO_LONG               -18009               //HPPT头过长

#define  HTTPCLIENT_MIN(x,y) (((x)<(y))?(x):(y))
#define  HTTPCLIENT_MAX(x,y) (((x)>(y))?(x):(y))

#define  HTTPCLIENT_AUTHB_SIZE     128

#define  HTTPCLIENT_CHUNK_SIZE     1152 //wenhe
#define  HTTPCLIENT_RAED_HEAD_SIZE 128            /* read header */
#define  HTTPCLIENT_SEND_BUF_SIZE  512
 
#define  HTTPCLIENT_MAX_HOST_LEN   64
#define  HTTPCLIENT_MAX_URL_LEN    512

#define  HTTP_RETRIEVE_MORE_DATA   (1)            /**< More data needs to be retrieved. */

/** @defgroup httpclient_define Define
 * @{
 */
/** @brief   This macro defines the HTTP port.  */
    
#define HTTP_PORT   80

/** @brief   This macro defines the HTTPS port.  */
#define HTTPS_PORT 443
/**
 * @}
 */

/** @defgroup httpclient_enum Enum
 *  @{
 */
/** @brief   This enumeration defines the HTTP request type.  */
typedef enum
{
    HTTPCLIENT_GET,
    HTTPCLIENT_POST,
    HTTPCLIENT_PUT,
    HTTPCLIENT_DELETE,
    HTTPCLIENT_HEAD
} HTTPCLIENT_REQUEST_TYPE;


/** @defgroup httpclient_struct Struct
 * @{
 */
/** @brief   This structure defines the httpclient_t structure.  */
typedef struct
{
    // INT32 socket;        /**< Socket ID. */
    //INT32 remote_port;      /**< HTTP or HTTPS port. */
    Network net;            //netWork for transport layer
    INT32 response_code;    /**< Response code. */
    INT8 *header;           /**< Custom header. */
    INT8 *auth_user;        /**< Username for basic authentication. */
    INT8 *auth_password;    /**< Password for basic authentication. */
} httpclient_t;

/** @brief   This structure defines the HTTP data structure.  */
typedef struct
{
    BOOL is_more;               /**< Indicates if more data needs to be retrieved. */
    BOOL is_chunked;            /**< Response data is encoded in portions/chunks.*/
    INT32 retrieve_len;         /**< Content length to be retrieved. */
    INT32 response_content_len; /**< Response content length. */
    INT32 post_buf_len;         /**< Post data length. */
    INT32 response_buf_len;     /**< Response buffer length. */
    INT8 *post_content_type;    /**< Content type of the post data. */
    INT8 *post_buf;             /**< User data to be posted. */
    INT8 *response_buf;         /**< Buffer to store the response data. */
} httpclient_data_t;

int  jiot_httpclient_parse_host(const char *url, char *scheme,UINT32 maxscheme_len, char *server, UINT32 maxserver_len,int *port);
int  jiot_httpclient_connect(httpclient_t *client);
int  jiot_httpclient_basic_auth(httpclient_t *client, char *user, char *password);
int  jiot_httpclient_send_request(httpclient_t *client, const char *url, HTTPCLIENT_REQUEST_TYPE method,httpclient_data_t *client_data);
int  jiot_httpclient_recv_response(httpclient_t *client, UINT32 timeout_ms, httpclient_data_t *client_data);
void jiot_httpclient_close(httpclient_t *client);
int  jiot_httpclient_get_response_code(httpclient_t *client);

#ifdef __cplusplus
}
#endif

#endif /* JIOT_HTTPCLIENT_H */

