#ifndef PKTIO_H_
#define PKTIO_H_

#include <odp.h>

odp_pktio_t pktio_create(const char *dev, odp_pool_t pool);
int pktio_send(odp_pktio_t pktio, odp_packet_t pkt[], int n);

#endif	/* PKTIO_H_ */
