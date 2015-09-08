#ifndef  _udp_servermsg_h
#define	_udp_servermsg_h

#include "udp_ringqueue.h"


typedef struct server_pthread_msg
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    ring_queue_t queue;
    volatile unsigned int task_num;

} server_pthread_msg_t;


server_pthread_msg_t * servermsg_new(unsigned int msg_num);
int servermsg_add(server_pthread_msg_t *server_msg, void *task);


#endif  /*_udp_servermsg_h*/