#ifndef _udp_serverpool_h
#define	_udp_serverpool_h




#define 	PTHREAD_MAX		(10)   
typedef struct udp_pthread_node
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	pthread_t  pid;
	struct sockaddr node_addres;
	void * data;
	unsigned char is_ok;
	unsigned char is_run;

}udp_pthread_node_t;


int pthread_pool_init(void * (*thread_fun)(void *));
int udp_register_client(struct sockaddr * client_addr);
int udp_unregister_client(struct sockaddr * client_addr);



#endif /*_udp_serverpool_h*/

