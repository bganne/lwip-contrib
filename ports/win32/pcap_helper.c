#include "pcap_helper.h"

#define WIN32_LEAN_AND_MEAN
/* get the windows definitions of the following 4 functions out of the way */
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#define HAVE_REMOTE
#include "pcap.h"

/** Get the index of an adapter by its network address
 *
 * @param netaddr network address of the adapter (e.g. 192.168.1.0)
 * @return index of the adapter or negative on error
 */
int
get_adapter_index_from_addr(u32_t netaddr, char *guid, u32_t guid_len)
{
   pcap_if_t *alldevs;
   pcap_if_t *d;
   char errbuf[PCAP_ERRBUF_SIZE+1];
   char source[PCAP_ERRBUF_SIZE+1];
   int index = 0;

   memset(guid, 0, guid_len);

   /* Retrieve the interfaces list */
   if (pcap_findalldevs_ex(source, NULL, &alldevs, errbuf) == -1) {
      printf("Error in pcap_findalldevs: %s\n", errbuf);
      return -1;
   }
   /* Scan the list printing every entry */
   for (d = alldevs; d != NULL; d = d->next, index++) {
      pcap_addr_t *a;
      for(a = d->addresses; a != NULL; a = a->next) {
         if (a->addr->sa_family == AF_INET) {
            u32_t a_addr = ((struct sockaddr_in *)a->addr)->sin_addr.s_addr;
            u32_t a_netmask = ((struct sockaddr_in *)a->netmask)->sin_addr.s_addr;
            u32_t a_netaddr = a_addr & a_netmask;
            if (a_netaddr == netaddr) {
               int ret = -1;
               char name[128];
               char *start, *end;
               strcpy(name, d->name);
               start = strstr(name, "{");
               if (start != NULL) {
                  end = strstr(start, "}");
                  if (end != NULL) {
                     size_t len = end - start + 1;
                     memcpy(guid, start, len);
                     ret = index;
                  }
               }
               pcap_freealldevs(alldevs);
               return ret;
            }
         }
      }
   }
   printf("Network address not found.\n");

   pcap_freealldevs(alldevs);
   return -1;
}
