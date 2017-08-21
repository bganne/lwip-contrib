#ifndef LWIP_ODPIF_H
#define LWIP_ODPIF_H

#include "lwip/netif.h"

err_t odpif_init(struct netif *netif);
#if NO_SYS
int odpif_select(struct netif *netif);
#endif /* NO_SYS */

#endif /* LWIP_ODPIF_H */
