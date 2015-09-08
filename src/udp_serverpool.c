#include "common.h"
#include "udp_serverpool.h"


#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"udp_serverpool:"


static udp_pthread_node_t  * pthread_node = NULL;



int pthread_pool_init(void * (*thread_fun)(void *))
{
	int i = 0;
	int ret = -1;
	if(NULL == thread_fun)
	{
		dbg_printf("thread_fun can not be null \n");
		return(-1);
	}
	if(NULL != pthread_node)
	{
		dbg_printf("init already \n");
		return(-1);
	}
	pthread_node = calloc(PTHREAD_MAX,sizeof(udp_pthread_node_t));
	if(NULL == pthread_node)
	{
		dbg_printf("calloc fail \n");
		return(-2);
	}
	for(i=0; i<PTHREAD_MAX; ++i)
	{
		pthread_node[i].pid = 0;
		pthread_node[i].is_ok=0;
		pthread_node[i].is_run=0;
		pthread_node[i].data = NULL;
		pthread_mutex_init(&pthread_node[i].mutex,NULL);
		pthread_cond_init(&pthread_node[i].cond,NULL);
		ret= pthread_create(&pthread_node[i].pid,NULL,thread_fun,&pthread_node[i]);
		if(ret == 0)
		{
			pthread_node[i].is_ok=1;
			pthread_detach(pthread_node[i].pid);
		}	
	}
	
	
	return(0);
}



int udp_register_client(struct sockaddr * client_addr)
{
	int i = 0;
	int result = -1;
	if(NULL == client_addr || NULL == pthread_node )
	{
		dbg_printf("check the param \n");
		return(-1);
	}

	for(i=0;i<PTHREAD_MAX; ++i)
	{
		if(0 == udp_sock_cmp_addr(pthread_node[i].data,client_addr))
		{
			result = i;
			break;
		}
	}
	if(result != -1)
	{
		dbg_printf("has register ! \n");
		return(-1);
	}
	
	for(i=0;i<PTHREAD_MAX; ++i)
	{
		if(pthread_node[i].is_ok==1 && pthread_node[i].is_run==0)
		{
			result = i;
			break;
		}
	}

	if(result == -1)return(-2);

	struct sockaddr * pclient = calloc(1,sizeof(struct sockaddr));
	if(NULL == pclient)
	{
		dbg_printf("calloc fail \n");
		return(-3);
	}
	memmove(pclient,client_addr,sizeof(struct sockaddr));
	pthread_mutex_lock(&pthread_node[result].mutex);
	pthread_node[result].data = pclient;
	pthread_node[i].is_run = 1;
	pthread_cond_signal(&pthread_node[result].cond);
	pthread_mutex_unlock(&pthread_node[result].mutex);
	return(0);

}


int udp_unregister_client(struct sockaddr * client_addr)
{
	int i = 0;
	int result = -1;
	if(NULL == client_addr || NULL == pthread_node)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	for(i=0;i<PTHREAD_MAX; ++i)
	{
		if(0 == udp_sock_cmp_addr(pthread_node[i].data,client_addr))
		{
			result = i;
			break;
		}
	}

	if(result == -1)return(-2);

	pthread_mutex_lock(&pthread_node[result].mutex);
	if( NULL != pthread_node[result].data)
	{
		free(pthread_node[result].data);
		pthread_node[result].data = NULL;
	}
	pthread_node[result].is_run = 0;
	pthread_mutex_unlock(&pthread_node[result].mutex);
	return(0);

}




static void * pthread_run(void * data)
{
	udp_pthread_node_t * node = (udp_pthread_node_t * )data;
	if(node == NULL)
	{
		dbg_printf("pthread_node not init \n");
		return(NULL);
	}
	while(1)
	{
		dbg_printf("pthread_id===%ld \n",udp_get_pid());
		if(NULL == node->data)
		{
			pthread_mutex_lock(&node->mutex);
			while(NULL == node->data)
			{
				pthread_cond_wait(&node->cond,&node->mutex);
			}
			/*block-->unlock-->wait()return-->lock*/
			pthread_mutex_unlock(&node->mutex);
		}
		char * pip_str = udp_sock_ntop((struct sockaddr *)node->data);
		dbg_printf("receive data from %s  port: %d \n",pip_str,ntohs(udp_get_port((struct sockaddr *)node->data)));
		if(NULL != pip_str)free(pip_str);
		sleep(2);

		
	}

	return(NULL);
}





