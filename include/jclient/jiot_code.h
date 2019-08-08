

#ifndef JIOT_CODE_H
#define JIOT_CODE_H


#define  JIOT_ERR_AUTH_ERR                           10001              //认证失败
#define  JIOT_ERR_PRODUCTKEY_OVERLONG                10002              //product_key超过长度
#define  JIOT_ERR_DEVICENAME_OVERLONG                10003              //deviceName超过长度
#define  JIOT_ERR_DEVICESECRET_OVERLONG              10004              //deviceSecret超过长度
#define  JIOT_ERR_TEXTFIELD_OVERLONG                 10005              //字段内容超过长度
#define  JIOT_ERR_SEQNO_ERROR                        10006              //序号错误
#define  JIOT_ERR_CODE_ERROR                         10007              //Code错误
#define  JIOT_ERR_TOPIC_FOTMAT_ERROR                 10008              //Topic格式错误
#define  JIOT_ERR_ARGU_FORMAT_ERROR                  10009              //参数格式错误
#define  JIOT_ERR_VERSION_FORMAT_ERROR               10010              //版本号格式错误
#define  JIOT_ERR_PROPERTY_FORMAT_ERROR              10011              //属性异常
#define  JIOT_ERR_PROPERTY_NAME_FORMAT_ERROR         10012              //属性名异常
#define  JIOT_ERR_PROPERTY_VALUE_FORMAT_ERROR        10013              //属性内容异常
#define  JIOT_ERR_EVENT_FORMAT_ERROR                 10014              //事件异常 
#define  JIOT_ERR_EVENT_NAME_FORMAT_ERROR            10015              //事件名异常
#define  JIOT_ERR_EVENT_CONTENT_FORMAT_ERROR         10016              //事件内容异常
#define  JIOT_ERR_DATA_CONTENT_FORMAT_ERROR          10017              //数据内容异常
#define  JIOT_ERR_VERSION_APP_VAR_FORMAT_ERROR       10018              //版本信息异常
  
#define  JIOT_ERR_MQTT_ERR                           11001              //MQTT异常
#define  JIOT_ERR_MQTT_PING_PACKET_ERROR             11002              //MQTT心跳异常
#define  JIOT_ERR_MQTT_NETWORK_ERROR                 11003              //MQTT网络异常
#define  JIOT_ERR_MQTT_CONNECT_PACKET_ERROR          11004              //MQTT连接异常
#define  JIOT_ERR_MQTT_PUBLISH_PACKET_ERROR          11005              //MQTT的PUBLISH消息异常
#define  JIOT_ERR_MQTT_PUSH_TO_LIST_ERROR            11006              //MQTT插入缓存队列异常
#define  JIOT_ERR_MQTT_PUBLISH_ACK_TYPE_ERROR        11007              //MQTT的PUB_ACK的类型异常
#define  JIOT_ERR_MQTT_PUBLISH_ACK_PACKET_ERROR      11008              //MQTT的PUB_ACK的数据异常
#define  JIOT_ERR_MQTT_SUBSCRIBE_PACKET_ERROR        11009              //MQTT的SUBSCRIBE异常
#define  JIOT_ERR_MQTT_UNSUBSCRIBE_PACKET_ERROR      11010              //MQTT的UNSUBSCRIBE异常
#define  JIOT_ERR_MQTT_PUBLISH_REC_PACKET_ERROR      11011              //MQTT的PUBLISH_REC异常
#define  JIOT_ERR_MQTT_PUBLISH_COMP_PACKET_ERROR     11012              //MQTT的PUBLISH_COMP异常
#define  JIOT_ERR_MQTT_STATE_ERROR                   11013              //MQTT状态异常
#define  JIOT_ERR_MQTT_NULL_VALUE_ERROR              11014              //MQTT检验值为空
#define  JIOT_ERR_MQTT_TOPIC_FORMAT_ERROR            11015              //MQTT的Topic格式异常
#define  JIOT_ERR_MQTT_PACKET_READ_ERROR             11016              //MQTT读失败
#define  JIOT_ERR_MQTT_CONNECT_ACK_PACKET_ERROR      11017              //MQTT的CONN_ACK消息读异常
#define  JIOT_ERR_MQTT_CONANCK_UNACCEPTABLE_PROTOCOL_VERSION_ERROR 11018  //MQTT使用的协议不匹配
#define  JIOT_ERR_MQTT_CONNACK_IDENTIFIER_REJECTED_ERROR 11019          //MQTT标识符被拒绝
#define  JIOT_ERR_MQTT_CONNACK_SERVER_UNAVAILABLE_ERROR  11020          //MQTT服务不可达
#define  JIOT_ERR_MQTT_CONNACK_BAD_USERDATA_ERROR    11021              //MQTT用户或密码错误
#define  JIOT_ERR_MQTT_CONNACK_NOT_AUTHORIZED_ERROR  11022              //MQTT客户端未授权
#define  JIOT_ERR_MQTT_CONNACK_UNKNOWN_ERROR         11023              //MQTT未知错误
#define  JIOT_ERR_MQTT_SUBSCRIBE_ACK_PACKET_ERROR    11024              //MQTT的SUB_ACK消息异常
#define  JIOT_ERR_MQTT_SUBSCRIBE_QOS_ERROR           11025              //MQTT的SUB_QOS异常
#define  JIOT_ERR_MQTT_SUB_INFO_NOT_FOUND_ERROR      11026              //MQTT未找到SUB_INFO异常
#define  JIOT_ERR_MQTT_NETWORK_CONNECT_ERROR         11027              //MQTT网络连接异常
#define  JIOT_ERR_MQTT_CONNECT_ERROR                 11028              //MQTT连接错误
#define  JIOT_ERR_MQTT_CREATE_THREAD_ERROR           11029              //MQTT创建线程异常
#define  JIOT_ERR_MQTT_PUBLISH_QOS_ERROR             11030              //MQTT的PUB消息QOS异常
#define  JIOT_ERR_MQTT_UNSUBSCRIBE_ACK_PACKET_ERROR  11031              //MQTT的UNSUB_ACK消息异常
#define  JIOT_ERR_MQTT_PACKET_BUFFER_TOO_SHORT 		 11032              //MQTT消息缓冲区不足
#define  JIOT_ERR_MQTT_PING_TIMEOUT_ERR              11033              //MQTT心跳超时

#define  JIOT_ERR_JCLI_ERR                           12001              //JCLI对象异常
#define  JIOT_ERR_JCLI_JSON_PARSE_ERR                12002              //JSON解析异常



#define  JIOT_ERR_SIS_HTTP_FAIL                     14001               //SIS HTTP错误
#define  JIOT_ERR_SIS_HTTP_CONN_FAIL                14002               //SIS HTTP连接失败
#define  JIOT_ERR_SIS_HTTP_REQ_FAIL                 14003               //SIS HTTP请求失败
#define  JIOT_ERR_SIS_CONTENT_ERROR                 14004               //SIS 数据内容错误
#define  JIOT_ERR_SIS_JSON_PARSE_FAIL               14005               //SIS Json解析错误



#endif
