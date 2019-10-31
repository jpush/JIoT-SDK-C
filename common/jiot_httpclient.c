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

#include "jiot_httpclient.h"

/*
static int httpclient_parse_url(const char *url, char *scheme, UINT32 max_scheme_len, char *host,UINT32 maxhost_len, int *port, char *path, UINT32 max_path_len);
static int httpclient_conn(httpclient_t *client);
static int httpclient_recv(httpclient_t *client, char *buf, int min_len, int max_len, int *p_read_len, UINT32 timeout);
static int httpclient_retrieve_content(httpclient_t *client, char *data, int len, UINT32 timeout, httpclient_data_t *client_data);
static int httpclient_response_parse(httpclient_t *client, char *data, int len, UINT32 timeout, httpclient_data_t *client_data);
*/

static void httpclient_base64enc(char *out, const char *in)
{
    const char code[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    int i = 0, x = 0, l = 0;

    for (; *in; in++) 
    {
        x = x << 8 | *in;
        for (l += 8; l >= 6; l -= 6) 
        {
            out[i++] = code[(x >> (l - 6)) & 0x3f];
        }
    }
    if (l > 0) 
    {
        x <<= 6 - l;
        out[i++] = code[x & 0x3f];
    }
    for (; i % 4;) 
    {
        out[i++] = '=';
    }
    out[i] = '\0';
}

int httpclient_conn(httpclient_t *client)
{
    if (0 != client->net._connect(&client->net)) 
    {
        ERROR_LOG("httpclient  connection failed");
        return JIOT_ERR_HTTP_CONN;
    }
    DEBUG_LOG("httpclient  connection success ");
    return JIOT_SUCCESS;
}

int httpclient_parse_url(const char *url, char *host, UINT32 maxhost_len, char *path, UINT32 max_path_len)
{
    /* parse the url (http[s]://host[:port][/[path]]) */
    char *host_ptr = (char *) strstr(url, "://");
    UINT32 host_len = 0;
    UINT32 path_len;
    /* char *port_ptr; */
    char *path_ptr = NULL;
    char *fragment_ptr = NULL;

    if (host_ptr == NULL) 
    {
        ERROR_LOG("Could not find host");
        return JIOT_ERR_HTTP_PARSE; /* URL is invalid */
    }  
    host_ptr += 3;

    path_ptr = strchr(host_ptr, '/');
    if (NULL == path_ptr) 
    {
        ERROR_LOG("invalid path");
        return JIOT_ERR_HTTP_PARSE;
    }

    host_len = path_ptr - host_ptr;
    
    if (maxhost_len < host_len + 1) 
    {
        ERROR_LOG("Host str is too long (host_len(%d) >= max_len(%d))", host_len + 1, maxhost_len);
        return JIOT_ERR_HTTP_PARSE;
    }
    memcpy(host, host_ptr, host_len);
    host[host_len] = '\0';

    fragment_ptr = strchr(host_ptr, '#');
    if (fragment_ptr != NULL) 
    {
        path_len = fragment_ptr - path_ptr;
    } 
    else 
    {
        path_len = strlen(path_ptr);
    }

    if (max_path_len < path_len + 1) 
    {
        ERROR_LOG("Path str is too small (%d >= %d)", max_path_len, path_len + 1);
        return JIOT_ERR_HTTP_PARSE;
    }
    memcpy(path, path_ptr, path_len);
    path[path_len] = '\0';

    return JIOT_SUCCESS;
}

int  jiot_httpclient_parse_host(const char *url, char *scheme,UINT32 max_scheme_len,  char *host_addr, UINT32 max_host_addr_len,int *port)
{
    char *scheme_ptr = (char *) url;
    char *host_ptr = (char *) strstr(url, "://");
    UINT32 host_addr_len = 0;
    char *path_ptr = NULL;

    if (host_ptr == NULL) 
    {
        ERROR_LOG("Could not find host");
        return JIOT_ERR_HTTP_PARSE; /* URL is invalid */
    }
    if (max_scheme_len < host_ptr - scheme_ptr + 1) 
    {
        /* including NULL-terminating char */
        ERROR_LOG("Scheme str is too small (%d >= %d)", max_scheme_len, (UINT32)(host_ptr - scheme_ptr + 1));
        return JIOT_ERR_HTTP_PARSE;
    }
    memcpy(scheme, scheme_ptr, host_ptr - scheme_ptr);
    scheme[host_ptr - scheme_ptr] = '\0';

    host_ptr += 3;

    char* port_ptr = strchr(host_ptr, ':');
    if (port_ptr != NULL)
    {
        UINT16 tport;
        host_addr_len = port_ptr - host_ptr;
        port_ptr++;
        if (sscanf(port_ptr, "%hu", &tport) != 1)
        {
            ERROR_LOG("Could not find port");
            return JIOT_ERR_HTTP_PARSE;
        }
        *port = (int) tport;
    }
    else
    {
        *port = 0;

        if (strcmp(scheme, "http") == 0)
        {
            *port = HTTP_PORT;
        }
        else if (strcmp(scheme, "https") == 0)
        {
            *port = HTTPS_PORT;
        }

        path_ptr = strchr(host_ptr, '/');
        host_addr_len = path_ptr - host_ptr;
    }

    if (max_host_addr_len < host_addr_len + 1) 
    {
        /* including NULL-terminating char */
        ERROR_LOG("Host str is too small (%d >= %d)", max_host_addr_len, host_addr_len + 1);
        return JIOT_ERR_HTTP_PARSE;
    }
    memcpy(host_addr, host_ptr, host_addr_len);
    host_addr[host_addr_len] = '\0';

    return JIOT_SUCCESS;
}

int httpclient_send(httpclient_t *client, char *data, int length)
{

    int ret = client->net._write(&client->net, (unsigned char *)data, length, 5000);    

    if (ret > 0) 
    {
        DEBUG_LOG("Written %d bytes", ret);
    } 
    else 
    if (ret == 0) 
    {
        ERROR_LOG("ret == 0,Connection was closed by server");
        return JIOT_ERR_HTTP_CLOSED; /* Connection was closed by server */
    } 
    else 
    {
        ERROR_LOG("Connection error (send returned %d)", ret);
        return JIOT_ERR_HTTP_ERROR;
    }
    return JIOT_SUCCESS;
} 

/* 0 on success, err code on failure */
int httpclient_recv(httpclient_t *client, char *buf,  int max_len, int *p_read_len, UINT32 timeout_ms)
{
    int ret = 0;

    *p_read_len = 0;
    ret = client->net._read(&client->net, (unsigned char *)buf, max_len, timeout_ms);

    //DEBUG_LOG("Recv: | %s", buf); 

    if (ret > 0)
    {
        *p_read_len = ret;
    } 
    else 
    if (ret == 0) 
    {
        return JIOT_FAIL;
    }
    else
    {
        ERROR_LOG("Connection error (recv returned %d)", ret);
        return JIOT_ERR_HTTP_ERROR;
    }
    
    return JIOT_SUCCESS;

}


/* 0 on success, err code on failure */
int httpclient_set_info(httpclient_t *client, char *send_buf, int *send_idx, char *buf,UINT32 len) 
{
    int ret = 0;
    int cp_len;
    int idx = *send_idx;

    if (len == 0) 
    {
        len = strlen(buf);
    }

    do 
    {
        if ((HTTPCLIENT_SEND_BUF_SIZE - idx) >= len) 
        {
            cp_len = len;
        }
        else 
        {
            cp_len = HTTPCLIENT_SEND_BUF_SIZE - idx;
        }

        memcpy(send_buf + idx, buf, cp_len);
        idx += cp_len;
        len -= cp_len;

        if (idx == HTTPCLIENT_SEND_BUF_SIZE) 
        {
            ret = client->net._write(&client->net, (unsigned char *)send_buf, HTTPCLIENT_SEND_BUF_SIZE, 5000);
            if (ret > 0) 
            {
                DEBUG_LOG("Written %d bytes", ret);
            } 
            else 
            if (ret == 0) 
            {
                ERROR_LOG("ret == 0,Connection was closed by server");
                return JIOT_ERR_HTTP_CLOSED; /* Connection was closed by server */
            } 
            else 
            {
                ERROR_LOG("Connection error (send returned %d)", ret);
                return JIOT_ERR_HTTP_ERROR;
            }
        }
    } while (len);

    *send_idx = idx;
    return JIOT_SUCCESS;
}

void jiot_httpclient_set_custom_header(httpclient_t *client, char *header)
{
    client->header = header;
}

int jiot_httpclient_basic_auth(httpclient_t *client, char *user, char *password)
{
    if ((strlen(user) + strlen(password)) >= HTTPCLIENT_AUTHB_SIZE) 
    {
        return JIOT_ERR_HTTP_ERROR;
    }
    client->auth_user = user;
    client->auth_password = password;
    return JIOT_SUCCESS;
}

int jiot_httpclient_set_auth(httpclient_t *client, char *send_buf, int *send_idx)
{
    char b_auth[(int)((HTTPCLIENT_AUTHB_SIZE + 3) * 4 / 3 + 1)];
    char base64buff[HTTPCLIENT_AUTHB_SIZE + 3];

    httpclient_set_info(client, send_buf, send_idx, "Authorization: Basic ", 0);
    sprintf(base64buff, "%s:%s", client->auth_user, client->auth_password);
    DEBUG_LOG("bAuth: %s", base64buff) ;
    httpclient_base64enc(b_auth, base64buff);
    b_auth[strlen(b_auth) + 1] = '\0';
    b_auth[strlen(b_auth)] = '\n';
    DEBUG_LOG("b_auth: %s", b_auth) ;
    httpclient_set_info(client, send_buf, send_idx, b_auth, 0);
    return JIOT_SUCCESS;
}

int jiot_httpclient_send_header(httpclient_t *client, const char *url, int method, httpclient_data_t *client_data)
{
    char host[HTTPCLIENT_MAX_HOST_LEN] = { 0 };
    char path[HTTPCLIENT_MAX_URL_LEN] = { 0 };
    int len;
    char send_buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    char buf[HTTPCLIENT_SEND_BUF_SIZE] = { 0 };
    char *meth = (method == HTTPCLIENT_GET) ? "GET" : (method == HTTPCLIENT_POST) ? "POST" :
                 (method == HTTPCLIENT_PUT) ? "PUT" : (method == HTTPCLIENT_DELETE) ? "DELETE" :
                 (method == HTTPCLIENT_HEAD) ? "HEAD" : "";
    int ret;

    /* First we need to parse the url (http[s]://host[:port][/[path]]) */
    /* int res = httpclient_parse_url(url, scheme, sizeof(scheme), host, sizeof(host), &(client->remote_port), path, sizeof(path)); */
    int res = httpclient_parse_url(url, host, sizeof(host), path, sizeof(path));
    if (res != JIOT_SUCCESS) 
    {
        ERROR_LOG("httpclient_parse_url fail returned %d", res);
        return res;
    }

    memset(send_buf, 0, HTTPCLIENT_SEND_BUF_SIZE);
    len = 0; /* Reset send buffer */

    snprintf(buf, sizeof(buf), "%s %s HTTP/1.1\r\nHost: %s\r\n", meth, path, host); /* Write request */

    ret = httpclient_set_info(client, send_buf, &len, buf, strlen(buf));
    if (ret) 
    {
        ERROR_LOG("Could not write request");
        return JIOT_ERR_HTTP_CONN;
    }

    /* Send all headers */
    if (client->auth_user) 
    {
        jiot_httpclient_set_auth(client, send_buf, &len); /* send out Basic Auth header */
    }

    /* Add user header information */
    if (client->header) 
    {
        httpclient_set_info(client, send_buf, &len, (char *) client->header, strlen(client->header));
    }

    if (client_data->post_buf != NULL) 
    {
        snprintf(buf, sizeof(buf), "Content-Length: %d\r\n", client_data->post_buf_len);
        httpclient_set_info(client, send_buf, &len, buf, strlen(buf));

        if (client_data->post_content_type != NULL) 
        {
            snprintf(buf, sizeof(buf), "Content-Type: %s\r\n", client_data->post_content_type);
            httpclient_set_info(client, send_buf, &len, buf, strlen(buf));
        }
    }

    /* Close headers */
    httpclient_set_info(client, send_buf, &len, "\r\n", 0);

    DEBUG_LOG("REQUEST : %s", send_buf);

    /* ret = httpclient_tcp_send_all(client->net.handle, send_buf, len); */
    ret = client->net._write(&client->net, (unsigned char *)send_buf, len, 5000);
    if (ret > 0) 
    {
        DEBUG_LOG("Written %d bytes", ret);
    }
    else 
    if (ret == 0) 
    {
        ERROR_LOG("ret == 0, Connection was closed by server");
        return JIOT_ERR_HTTP_CLOSED; /* Connection was closed by server */
    } 
    else 
    {
        ERROR_LOG("Connection error (send returned %d)", ret);
        return JIOT_ERR_HTTP_CONN;
    }

    return JIOT_SUCCESS;
}

int jiot_httpclient_send_userdata(httpclient_t *client, httpclient_data_t *client_data)
{
    int ret = 0;

    if (client_data->post_buf && client_data->post_buf_len)
     {
        DEBUG_LOG("client_data->post_buf: %s", client_data->post_buf);
        {
            /* ret = httpclient_tcp_send_all(client->handle, (char *)client_data->post_buf, client_data->post_buf_len); */
            ret = client->net._write(&client->net, (unsigned char *)client_data->post_buf, client_data->post_buf_len, 5000);
            if (ret > 0) 
            {
                DEBUG_LOG("Written %d bytes", ret);
            }
            else 
            if (ret == 0) 
            {
                ERROR_LOG("ret == 0, Connection was closed by server");
                return JIOT_ERR_HTTP_CLOSED; /* Connection was closed by server */
            } 
            else 
            {
                ERROR_LOG("Connection error (send returned %d)", ret);
                return JIOT_ERR_HTTP_CONN;
            }
        }
    }

    return JIOT_SUCCESS;
}

int httpclient_retrieve_content(httpclient_t *client, char *data, int len,
                                UINT32 timeout_ms, httpclient_data_t *client_data)
{
    int count = 0;
    int templen = 0;
    int crlf_pos;
    Timer timer;

    InitTimer(&timer);
    countdown_ms(&timer, timeout_ms);

    /* Receive data */
//    DEBUG_LOG("Current data: %s", data);

    client_data->is_more = JIOT_TRUE;

    /* the header is not received finished */
    if (client_data->response_content_len == -1 && client_data->is_chunked == JIOT_FALSE) 
    {
        /* can not enter this if */
        while (1) 
        {
            int ret, max_len;
            if (count + len < client_data->response_buf_len - 1) 
            {
                memcpy(client_data->response_buf + count, data, len);
                count += len;
                client_data->response_buf[count] = '\0';
            } 
            else 
            {
                memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
                client_data->response_buf[client_data->response_buf_len - 1] = '\0';
                return HTTP_RETRIEVE_MORE_DATA;
            }

            /* try to read more header */
            max_len = HTTPCLIENT_MIN(HTTPCLIENT_RAED_HEAD_SIZE, client_data->response_buf_len - 1 - count);
            ret = httpclient_recv(client, data, max_len, &len, left_ms(&timer));

            /* Receive data */
            DEBUG_LOG("data len: %d %d", len, count);

            if (ret != JIOT_SUCCESS) 
            {
                return ret;
            }

            if (len == 0) 
            {
                /* read no more data */
                DEBUG_LOG("no more len == 0");
                client_data->is_more = JIOT_FALSE;
                return JIOT_SUCCESS;
            }
        }
    }

    while (1) 
    {
        UINT32 readLen = 0;

        if (client_data->is_chunked && client_data->retrieve_len <= 0) 
        {
            /* Read chunk header */
            int foundCrlf;
            int n;
            do 
            {
                foundCrlf = JIOT_FALSE;
                crlf_pos = 0;
                data[len] = 0;
                if (len >= 2) 
                {
                    for (; crlf_pos < len - 2; crlf_pos++) 
                    {
                        if (data[crlf_pos] == '\r' && data[crlf_pos + 1] == '\n') 
                        {
                            foundCrlf = JIOT_TRUE;
                            break;
                        }
                    }
                }
                if (!foundCrlf) 
                {
                    /* Try to read more */
                    if (len < HTTPCLIENT_CHUNK_SIZE) 
                    {
                        int new_trf_len, ret;
                        ret = httpclient_recv(client,
                                              data + len,
                                              HTTPCLIENT_CHUNK_SIZE - len - 1,
                                              &new_trf_len,
                                              left_ms(&timer));
                        len += new_trf_len;
                        if (ret != JIOT_SUCCESS) 
                        {
                            return ret;
                        } 
                        else 
                        {
                            continue;
                        }
                    } 
                    else 
                    {
                        return JIOT_ERR_HTTP_ERROR;
                    }
                }
            } while (!foundCrlf);
            data[crlf_pos] = '\0';

            /* chunk length */
            /* n = sscanf(data, "%x", &readLen); */

            readLen = strtoul(data, NULL, 16);
            n = (0 == readLen) ? 0 : 1;
            client_data->retrieve_len = readLen;
            client_data->response_content_len += client_data->retrieve_len;
            if (readLen == 0) 
            {
                /* Last chunk */
                client_data->is_more = JIOT_FALSE;
                DEBUG_LOG("no more (last chunk)");
            }

            if (n != 1) 
            {
                ERROR_LOG("Could not read chunk length");
                return JIOT_ERR_HTTP_UNRESOLVED_DNS;
            }

            memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2)); /* Not need to move NULL-terminating char any more */
            len -= (crlf_pos + 2);
        } 
        else 
        {
            readLen = client_data->retrieve_len;
        }

//        DEBUG_LOG("Total-Payload: %d Bytes; Read: %d Bytes", readLen, len);

        do 
        {
            templen = HTTPCLIENT_MIN(len, readLen);
            if (count + templen < client_data->response_buf_len - 1) 
            {
                memcpy(client_data->response_buf + count, data, templen);
                count += templen;
                client_data->response_buf[count] = '\0';
                client_data->retrieve_len -= templen;
            } 
            else 
            {
                memcpy(client_data->response_buf + count, data, client_data->response_buf_len - 1 - count);
                client_data->response_buf[client_data->response_buf_len - 1] = '\0';
                client_data->retrieve_len -= (client_data->response_buf_len - 1 - count);
                return HTTP_RETRIEVE_MORE_DATA;
            }

            if (len > readLen) 
            {
                DEBUG_LOG("memmove %d %d %d\n", readLen, len, client_data->retrieve_len);
                memmove(data, &data[readLen], len - readLen); /* chunk case, read between two chunks */
                len -= readLen;
                readLen = 0;
                client_data->retrieve_len = 0;
            } 
            else 
            {
                readLen -= len;
            }

            if (readLen) 
            {
                int ret;
                int max_len = HTTPCLIENT_MIN(HTTPCLIENT_CHUNK_SIZE - 1, client_data->response_buf_len - 1 - count);
                max_len = HTTPCLIENT_MIN(max_len, readLen);
                ret = httpclient_recv(client, data, max_len, &len, left_ms(&timer));
                if (ret != JIOT_SUCCESS) 
                {
                    return ret;
                }
            }
        } while (readLen);

        if (client_data->is_chunked)
        {
            if (len < 2) 
            {
                int new_trf_len, ret;
                /* Read missing chars to find end of chunk */
                ret = httpclient_recv(client, data + len, HTTPCLIENT_CHUNK_SIZE - len - 1, &new_trf_len,
                                      left_ms(&timer));
                if (ret != JIOT_SUCCESS) 
                {
                    return ret;
                }
                len += new_trf_len;
            }
            if ((data[0] != '\r') || (data[1] != '\n')) 
            {
                ERROR_LOG("Format error, %s", data); /* after memmove, the beginning of next chunk */
                return JIOT_ERR_HTTP_UNRESOLVED_DNS;
            }
            memmove(data, &data[2], len - 2); /* remove the \r\n */
            len -= 2;
        } 
        else 
        {
            DEBUG_LOG("no more (content-length)");
            client_data->is_more = JIOT_FALSE;
            break;
        }

    }

    return JIOT_SUCCESS;
}

int httpclient_response_parse(httpclient_t *client, char *data, int len, UINT32 timeout_ms,
                              httpclient_data_t *client_data)
{
    int crlf_pos;
    Timer timer;
    char *tmp_ptr, *ptr_body_end;

    int new_trf_len, ret;

    InitTimer(&timer);
    countdown_ms(&timer, timeout_ms);

    client_data->response_content_len = -1;

    /* http client response */
    /* <status-line> HTTP/1.1 200 OK(CRLF)

       <headers> ...(CRLF)

       <blank line> (CRLF)

      [<response-body>] */
    char *crlf_ptr = strstr(data, "\r\n");
    if (crlf_ptr == NULL) 
    {
        ERROR_LOG("\\r\\n not found");
        return JIOT_ERR_HTTP_UNRESOLVED_DNS;
    }

    crlf_pos = crlf_ptr - data;
    data[crlf_pos] = '\0';

    /* Parse HTTP response */

    if (sscanf(data, "HTTP/%*d.%*d %d %*[^\r\n]", &(client->response_code)) != 1) 
    {
        /* Cannot match string, error */
        ERROR_LOG("Not a correct HTTP answer : %s\n", data);
        return JIOT_ERR_HTTP_UNRESOLVED_DNS;
    }

    if ((client->response_code < 200) || (client->response_code >= 400)) 
    {
        /* Did not return a 2xx code; TODO fetch headers/(&data?) anyway and implement a mean of writing/reading headers */
        ERROR_LOG("Response code %d", client->response_code);
    }

    DEBUG_LOG("Reading headers: %s", data);

    memmove(data, &data[crlf_pos + 2], len - (crlf_pos + 2) + 1); /* Be sure to move NULL-terminating char as well */
    len -= (crlf_pos + 2);       /* remove status_line length */

    client_data->is_chunked = JIOT_FALSE;

    /*If not ending of response body*/
    /* try to read more header again until find response head ending "\r\n\r\n" */
    while (NULL == (ptr_body_end = strstr(data, "\r\n\r\n"))) 
    {
    	/* check read length, against buf overflow */
        if(len >= HTTPCLIENT_CHUNK_SIZE - HTTPCLIENT_RAED_HEAD_SIZE)
        {
        	return JIOT_ERR_HTTP_HEADER_TOO_LONG;
        }

        /* try to read more header */
        ret = httpclient_recv(client, data + len,  HTTPCLIENT_RAED_HEAD_SIZE, &new_trf_len, left_ms(&timer));
        if (ret != JIOT_SUCCESS) 
        {
            return ret;
        }
        len += new_trf_len;
        data[len] = '\0';
    }

    /* parse response_content_len */
    if (NULL != (tmp_ptr = strstr(data, "Content-Length"))) 
    {
        client_data->response_content_len = atoi(tmp_ptr + strlen("Content-Length: "));
        client_data->retrieve_len = client_data->response_content_len;
    } 
    else 
    if (NULL != (tmp_ptr = strstr(data, "Transfer-Encoding"))) 
    {
        int len_chunk = strlen("Chunked");
        char *chunk_value = data + strlen("Transfer-Encoding: ");

        if ((! memcmp(chunk_value, "Chunked", len_chunk))
            || (! memcmp(chunk_value, "chunked", len_chunk))) 
        {
            client_data->is_chunked = JIOT_TRUE;
            client_data->response_content_len = 0;
            client_data->retrieve_len = 0;
        }
    } 
    else 
    {
        ERROR_LOG("Could not parse header");
        return JIOT_ERR_HTTP_ERROR;
    }

    /* remove header length */
    /* len is Had read body's length */
    /* if client_data->response_content_len != 0, it is know response length */
    /* the remain length is client_data->response_content_len - len */
    len = len - (ptr_body_end + 4 - data);
    memmove(data, ptr_body_end + 4, len + 1);
    // client_data->response_received_len += len;
    return httpclient_retrieve_content(client, data, len, left_ms(&timer), client_data);
}

int jiot_httpclient_connect(httpclient_t *client)
{
    int ret = JIOT_ERR_HTTP_CONN;

    client->net._socket = -1;

    ret = httpclient_conn(client);

    return ret;
}

int jiot_httpclient_send_request(httpclient_t *client, const char *url, HTTPCLIENT_REQUEST_TYPE method,httpclient_data_t *client_data)
{
    int ret = JIOT_ERR_HTTP_CONN;

    if (-1 == client->net._socket) 
    {
        ERROR_LOG("not connection have been established");
        return ret;
    }

    ret = jiot_httpclient_send_header(client, url, method, client_data);
    if (ret != 0) 
    {
        ERROR_LOG("httpclient_send_header is error,ret = %d", ret);
        return ret;
    }

    if (method == HTTPCLIENT_POST || method == HTTPCLIENT_PUT) 
    {
        ret = jiot_httpclient_send_userdata(client, client_data);
    }

    return ret;
}

int jiot_httpclient_recv_response(httpclient_t *client, UINT32 timeout_ms, httpclient_data_t *client_data)
{
    int reclen = 0, ret = JIOT_ERR_HTTP_CONN;
    char buf[HTTPCLIENT_CHUNK_SIZE] = { 0 };
    Timer timer;

    InitTimer(&timer);
    countdown_ms(&timer, timeout_ms);

    if (-1 == client->net._socket) 
    {
        ERROR_LOG("not connection have been established");
        return ret;
    }

    if (client_data->is_more) 
    {
        client_data->response_buf[0] = '\0';
        ret = httpclient_retrieve_content(client, buf, reclen, left_ms(&timer), client_data);
    } 
    else 
    {
        client_data->is_more = 1;
        /* try to read header */
        ret = httpclient_recv(client, buf,  HTTPCLIENT_RAED_HEAD_SIZE, &reclen, left_ms(&timer));
        if (ret != JIOT_SUCCESS) 
        {
            return ret;
        }

        buf[reclen] = '\0';

        if (reclen) 
        {
            DEBUG_LOG("RESPONSE :%s", buf);
            ret = httpclient_response_parse(client, buf, reclen, left_ms(&timer), client_data);
        }
    }

    return ret;
}

void jiot_httpclient_close(httpclient_t *client)
{
    if (client->net._socket >= 0) 
    {
        client->net._disconnect(&client->net);
    }
    client->net._socket = -1;
    DEBUG_LOG("client disconnected");
}


int jiot_httpclient_get_response_code(httpclient_t *client)
{
    return client->response_code;
}


