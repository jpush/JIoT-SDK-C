#ifndef JIOT_NETWORK_DEFINE_H
#define JIOT_NETWORK_DEFINE_H



#include "jiot_network.h"

#ifdef JIOT_SSL
#include "jiot_network_ssl.h"
#endif

typedef struct {
    char *pHostAddress;                 ///< Pointer to a string defining the endpoint for the service
    char *pHostPort;                    ///< service listening port
    char *pPubKey;                      ///< Pointer to a string defining the Root CA file (full file, not path)
    E_NET_TYPE eNetworkType;
}ConnectParams;


typedef struct Network Network;
#ifndef JIOT_SSL
struct Network
{
    int _socket;                                               /**< Connect the socket handle. */
    int (*_read)(Network *, unsigned char *, int, int);        /**< Read data from server function pointer. */
    int (*_write)(Network *, unsigned char *, int, int);       /**< Send data to server function pointer. */
    void(*_disconnect)(Network *);                             /**< Disconnect the network function pointer.*/
    int (*_connect)(Network *);
    ConnectParams connectparams;
} ;
#else
struct Network
{
    int _socket;                                               /**< noused  */ 
    int (*_read)(Network *, unsigned char *, int, int);        /**< Read data from server function pointer. */
    int (*_write)(Network *, unsigned char *, int, int);       /**< Send data to server function pointer. */
    void(*_disconnect)(Network *);                             /**< Disconnect the network function pointer.*/
    int (*_connect)(Network *);
    ConnectParams connectparams;
    TLSDataParams tlsdataparams;                               /**< Connect the SSL socket handle. */
} ;
#endif

#endif