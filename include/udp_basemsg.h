#ifndef _udp_basemsg_h
#define _udp_basemsg_h

typedef  enum  msg_type
{
	LOGIN_MSG,
	LOGIN_ASK_MSG,
	SYNC_MSG,
	SYNC_ASK_MSG,
	KEEP_ALIVE_MSG,
	KEEP_ALIVE_ASK_MSG,

}msg_type_t;



typedef struct msg_header
{
	msg_type_t type;
	struct sockaddr src_addr;
	struct sockaddr dst_addr;
	
}msg_header_t;





#endif  /*_udp_basemsg_h*/