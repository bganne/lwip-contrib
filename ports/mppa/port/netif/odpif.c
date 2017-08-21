#include <unistd.h>
#include <odp.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"

#include "netif/odpif.h"
#include "netif/pktio.h"

#define MTU 9000
#define PKTBUF 100

/* Forward declarations. */
static void odpif_input(struct netif *netif);
#if !NO_SYS
static void odpif_thread(void *arg);
#endif /* !NO_SYS */

struct odpif {
	odp_pktio_t pktio;
	odp_pool_t pool;
	const char *name;
};

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif)
{
  int err;
  odp_pool_param_t params;
  struct odpif *odpif = (struct odpif *)netif->state;

  LWIP_DEBUGF(NETIF_DEBUG, ("odpif_init for iface '%s'.\n", odpif->name));

  err = odp_init_global(NULL, NULL);
  if (err) {
	  perror("odpif_init: odp_init_global() failed");
	  exit(1);
  }

  err = odp_init_local(
#if NO_SYS
		  ODP_THREAD_CONTROL
#else	/* !NO_SYS */
		  ODP_THREAD_WORKER
#endif /* NO_SYS */
		  );
  if (err) {
	  perror("odpif_init: odp_init_local() failed");
	  exit(1);
  }

  odp_pool_param_init(&params);
  params.pkt.seg_len = MTU;
  params.pkt.len     = MTU;
  params.pkt.num     = PKTBUF;
  params.type        = ODP_POOL_PACKET;
  odpif->pool = odp_pool_create("packet_pool", &params);
  if (ODP_POOL_INVALID == odpif->pool) {
	  perror("odpif_init: odp_pool_create() failed");
	  exit(1);
  }
#if NETIF_DEBUG
  odp_pool_print(odpif->pool);
#endif /* NETIF_DEBUG */

  odpif->pktio = pktio_create(odpif->name, odpif->pool);

  err = odp_pktio_mac_addr(odpif->pktio, netif->hwaddr, sizeof(netif->hwaddr));
  if (err < 0) {
	  perror("odpif_init: odp_pktio_mac_addr() failed");
	  exit(1);
  }
  netif->hwaddr_len = 6;	/* eth */

  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  netif_set_link_up(netif);

#if !NO_SYS
  sys_thread_new("odpif_thread", odpif_thread, netif, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#endif /* !NO_SYS */
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  int err;
  odp_packet_t pkt;
  struct odpif *odpif = (struct odpif *)netif->state;

  pkt = odp_packet_alloc(odpif->pool, p->tot_len);
  if (ODP_PACKET_INVALID == pkt) {
	perror("odpif: odp_packet_alloc() failed");
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
	return ERR_MEM;
  }

  pbuf_copy_partial(p, odp_packet_data(pkt), p->tot_len, 0);

  err = pktio_send(odpif->pktio, &pkt, 1);
  if (1 != err) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
	return ERR_IF;
  }

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  return ERR_OK;
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif)
{
  int rx;
  odp_packet_t pkt;
  uint32_t len;
  const void *eth;
  struct pbuf *p;
  struct odpif *odpif = (struct odpif *)netif->state;

  while (0 == (rx = odp_pktio_recv(odpif->pktio, &pkt, 1)));
  if (rx < 0) {
	  perror("odpif: odp_pktio_recv() failed");
	  exit(1);
  }

  LWIP_DEBUGF(NETIF_DEBUG, ("odpif_input: rx packet\n"));

  eth = odp_packet_l2_ptr(pkt, &len)
  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {
    pbuf_take(p, eth, len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("odpif_input: could not allocate pbuf\n"));
  }

  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * odpif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
odpif_input(struct netif *netif)
{
  struct pbuf *p = low_level_input(netif);

  if (p == NULL) {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(NETIF_DEBUG, ("odpif_input: low_level_input returned NULL\n"));
    return;
  }

  if (netif->input(p, netif) != ERR_OK) {
    LWIP_DEBUGF(NETIF_DEBUG, ("odpif_input: netif input error\n"));
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * odpif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
odpif_init(struct netif *netif)
{
  struct odpif *odpif = (struct odpif *)mem_malloc(sizeof(struct odpif));
  if (odpif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("odpif_init: out of memory for odpif\n"));
    return ERR_MEM;
  }
  odpif->name = strdup((const char *)netif->state);
  if (odpif->name == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("odpif_init: out of memory for odpif\n"));
    return ERR_MEM;
  }
  netif->state = odpif;
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);

  netif->name[0] = 'e';
  netif->name[1] = 'X';

#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  netif->mtu = MTU;

  low_level_init(netif);

  return ERR_OK;
}


/*-----------------------------------------------------------------------------------*/
#if NO_SYS

int
odpif_select(struct netif *netif)
{
  odpif_input(netif);
  return 0;
}

#else /* !NO_SYS */

static void
odpif_thread(void *arg)
{
  int err = odp_init_local(ODP_THREAD_WORKER);
  if (err) {
	  perror("odpif_thread: odp_init_local() failed");
	  exit(1);
  }

  struct netif *netif = (struct netif *)arg;
  for (;;) {
	  /* Handle incoming packet. */
	  odpif_input(netif);
  }
}

#endif /* NO_SYS */
