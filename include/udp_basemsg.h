#ifndef _udp_basemsg_h
#define _udp_basemsg_h

#include "common.h"
#include "udp_ringqueue.h"
#include "udp_tailqueue.h"

typedef  enum  msg_type
{
	LOGIN_MSG,
	LOGIN_ASK_MSG,
	SYNC_MSG,
	SYNC_ASK_MSG,
	KEEP_ALIVE_MSG,
	KEEP_ALIVE_ASK_MSG,

}msg_type_t;



typedef struct msg_data
{
	msg_type_t type;
	unsigned char is_reliable;
	struct sockaddr src_addr;
	struct sockaddr dst_addr;
	unsigned long packer_index;
	unsigned int data_length;
	unsigned int retry_times;
	char * data;
}msg_data_t;

typedef struct reliable_packet
{
	msg_data_t 	msg;
	unsigned long time_stamp_start;
	unsigned long time_stamp_end;
	unsigned long time_stamp_real_end;
	
}reliable_packet_t;


typedef struct  packet_node 
{
    reliable_packet_t * packet;
    TAILQ_ENTRY(packet_node) links;  
}packet_node_t;


typedef struct packet_list
{
	pthread_mutex_t  mutex;
	volatile unsigned int num_packets;
	TAILQ_HEAD(,packet_node)packet_queue;
	
}packet_list_t;



typedef struct server_pthread_msg
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    ring_queue_t handle_msg_queue;
    ring_queue_t send_msg_queue;
    ring_queue_t resend_msg_queue;
    volatile unsigned int handle_msg_num;
    volatile unsigned int send_msg_num;
    volatile unsigned int resend_msg_num;

} server_pthread_msg_t;


typedef struct server_session
{
	int socket_fd;
	int wakeup_fd;
	unsigned long packets_index;
	struct sockaddr_in	servaddr;
	server_pthread_msg_t * msg_pool;
	packet_list_t	* reliable_queue;
}server_session_t;


typedef struct handle_msg_fun
{
	msg_type_t type;
	int (*handle)(server_session_t * ,void * );
		
}handle_msg_fun_t;








#endif  /*_udp_basemsg_h*/
