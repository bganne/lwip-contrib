#ifndef LWIP_DUMMYIF_H
#define LWIP_DUMMYIF_H

#include "lwip/netif.h"

err_t dummyif_init(struct netif *netif);
#if NO_SYS
int dummyif_select(struct netif *netif);
#endif /* NO_SYS */

#endif /* LWIP_DUMMYIF_H */
