#include <stdio.h>
#include <odp.h>
#include "netif/pktio.h"

odp_pktio_t pktio_create(const char *dev, odp_pool_t pool)
{
	odp_pktio_param_t pktio_param;
	odp_pktio_t pktio;
	int err;

	odp_pktio_param_init(&pktio_param);
	pktio_param.in_mode = ODP_PKTIN_MODE_RECV;
	pktio_param.out_mode = ODP_PKTOUT_MODE_SEND;
	pktio = odp_pktio_open(dev, pool, &pktio_param);
	if (ODP_PKTIO_INVALID == pktio) {
		perror("odp_pktio_open() failed");
		exit(1);
	}

	err = odp_pktio_start(pktio);
	if (0 != err) {
		perror("odp_pktio_start() failed");
		exit(1);
	}

	return pktio;
}

int pktio_send(odp_pktio_t pktio, odp_packet_t pkt[], int n)
{
	int tx = odp_pktio_send(pktio, pkt, n);
	if (tx != n) {
		int i;
		fprintf(stderr, "odp_pktio_send() failed (%i out of %i packets sent).\n", tx, n);
		tx = tx > 0 ? tx : 0;
		for (i=tx; i<n; i++) {
			odp_packet_free(pkt[i]);
		}
	}
	return tx;
}
