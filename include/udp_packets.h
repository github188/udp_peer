#ifndef _udp_packets_h
#define	_udp_packets_h
#include "udp_basemsg.h"


packet_list_t * udp_packets_new(void);
int udp_get_packets_num(packet_list_t * list);
int udp_push_packets(packet_list_t * list,packet_node_t * node );
packet_node_t * udp_get_first_packet(packet_list_t * list);
reliable_packet_t * udp_find_packets(packet_list_t * list,unsigned long time_stamp);
unsigned long udp_find_and_delpackets(packet_list_t * list,unsigned long time_stamp);


#endif /*_udp_packets_h*/