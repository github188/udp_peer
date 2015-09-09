#ifndef  _udp_servermsg_h
#define	_udp_servermsg_h

#include "udp_basemsg.h"





server_pthread_msg_t * servermsg_new(unsigned int msg_num);
int servermsg_add_handle_msg(server_pthread_msg_t *server_msg, void *task);
int servermsg_add_send_msg(server_pthread_msg_t *server_msg, void *task);
int servermsg_add_resend_msg(server_pthread_msg_t *server_msg, void *task);


#endif  /*_udp_servermsg_h*/