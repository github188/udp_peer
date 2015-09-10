#include "common.h"
#include "udp_servermsg.h"
#include "udp_basemsg.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"main_server:"


#undef 	MAXLINE
#define MAXLINE   1024

#define SERVER_PORT		(45620)
#define	MSG_MAX_NUM		(1024)
#define	NET_INTERFACE	"eth0"




static  server_session_t * session_server = NULL;


int session_server_init(void)
{

	int ret = -1;
	int size = 220 * 1024;
	if(NULL != session_server)
	{
		dbg_printf("init already!\n");
		return(-1);
	}
	session_server = calloc(1,sizeof(server_session_t));
	if(NULL == session_server)
	{
		dbg_printf("calloc fail \n");
		return(-2);
	}

	session_server->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(session_server->socket_fd < 0)
	{
		dbg_printf("socket fail \n");
		goto fail;
	}

	bzero(&session_server->servaddr, sizeof(struct sockaddr_in));
	session_server->servaddr.sin_family      = AF_INET;
	session_server->servaddr.sin_addr.s_addr = udp_get_localaddres(NET_INTERFACE);
	session_server->servaddr.sin_port        = htons(SERVER_PORT);

	setsockopt(session_server->socket_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	setsockopt(session_server->socket_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	ret = bind(session_server->socket_fd, (struct sockaddr *) &session_server->servaddr, sizeof(struct sockaddr));
	if(ret != 0 )
	{
		dbg_printf("bing fail \n");
		goto fail;
	}

	session_server->msg_pool = servermsg_new(MSG_MAX_NUM);
	if(NULL == session_server->msg_pool)
	{
		dbg_printf("servermsg_new fail \n");
		goto fail;
	}

	return(0);

fail:



	if(session_server->socket_fd  > 0)
	{
		udp_shutdown_socket(session_server->socket_fd,SHUTDOWN_READ_WRITE);
	}

	if(NULL != session_server)
	{
		free(session_server);
		session_server = NULL;
	}
	return(-3);
}


int servermsg_handle_login(server_session_t * session,void * data)
{
	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	msg_data_t * msg = (msg_data_t *)data;

	char * pip_str = udp_sock_ntop(&msg->src_addr);
	dbg_printf("receive data from %s  port: %d \n",pip_str,ntohs(udp_get_port(&msg->src_addr)));
	if(NULL != pip_str)free(pip_str);	

	msg_data_t * ask_msg = calloc(1,sizeof(msg_data_t));
	if(NULL != ask_msg)
	{
		ask_msg->type = LOGIN_ASK_MSG;
		memmove(&ask_msg->dst_addr,&msg->src_addr,sizeof(struct sockaddr));
		ask_msg->time_stamp_end = msg->time_stamp_end;
		servermsg_add_send_msg(session->msg_pool,ask_msg);
	}

	free(data);
	data = NULL;


	return(0);

}

int servermsg_handle_login_ask(server_session_t * session,void * data)
{
	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	msg_data_t * msg = (msg_data_t *)data;
	sendto(session->socket_fd, msg, sizeof(msg_data_t), 0, &msg->dst_addr, sizeof(struct sockaddr));

	free(data);
	data = NULL;
	


	return(0);

}


static handle_msg_fun_t  servermsg_fun[]=\
{
	{LOGIN_MSG,servermsg_handle_login},
	{LOGIN_ASK_MSG,servermsg_handle_login_ask},
};



static void* handleserver_msg_pool_loop(void *arg)
{

	int ret = -1;
	int i = 0;
	void * task = NULL;
	server_session_t * server = (server_session_t *)arg;
	if(NULL == server || NULL == server->msg_pool)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
    while (1)
    {
        pthread_mutex_lock(&(server->msg_pool->mutex));
 
        while ((0 == server->msg_pool->handle_msg_num)&&(0 == server->msg_pool->send_msg_num)&&(0 == server->msg_pool->resend_msg_num))
        {
            pthread_cond_wait(&(server->msg_pool->cond), &(server->msg_pool->mutex));
        }

		if(0 != server->msg_pool->resend_msg_num)
		{

			ret = ring_queue_pop(&(server->msg_pool->resend_msg_queue), &task);
			pthread_mutex_unlock(&(server->msg_pool->mutex));
			if(ret != 0)continue;

			
			
	        volatile unsigned int *resend_num = &(server->msg_pool->resend_msg_num);
    		fetch_and_sub(resend_num, 1);  

			
		}
		else if(0 != server->msg_pool->handle_msg_num)
		{

			ret = ring_queue_pop(&(server->msg_pool->handle_msg_queue), &task);
			pthread_mutex_unlock(&(server->msg_pool->mutex));
			if(ret != 0)continue;
						
	        volatile unsigned int *handle_num = &(server->msg_pool->handle_msg_num);
    		fetch_and_sub(handle_num, 1);  

		}
		else if(0 != server->msg_pool->send_msg_num)
		{

			ret = ring_queue_pop(&(server->msg_pool->send_msg_queue), &task);
			pthread_mutex_unlock(&(server->msg_pool->mutex));
			if(ret != 0)continue;
			
	        volatile unsigned int *send_num = &(server->msg_pool->send_msg_num);
    		fetch_and_sub(send_num, 1);  
		}
		else
		{
			pthread_mutex_unlock(&(server->msg_pool->mutex));
			continue;

		}

		
		if(NULL != task)
		{
			msg_data_t * msg = (msg_data_t *)task;
			int find_fun  = 0;
			for(i=0;i<sizeof(servermsg_fun)/sizeof(servermsg_fun[0]);++i)
			{
				if(msg->type == servermsg_fun[i].type)
				{
					find_fun = 1;
					servermsg_fun[i].handle(server,msg);
					break;
				}
			}

			if(0 == find_fun)
			{
				free(task);
				task = NULL;
			}

		}
	
    }

    pthread_exit(NULL);
    return NULL;
}



void udp_recv_message(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];

	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		if(n <= 0 )continue;
		if(n < sizeof(msg_data_t))continue;
		
		void * data = calloc(1,sizeof(unsigned char)*(n+1));
		if(NULL == data)continue;
		
		
		memmove(data,mesg,n);
		servermsg_add_handle_msg(session_server->msg_pool, data);

	}
}


int main(int argc, char **argv)
{

	int ret = -1;
	struct sockaddr_in	cliaddr;
	ret = session_server_init();
	if(ret != 0)
	{
		dbg_printf("session_server_init fail\n");
		return(-1);
	}

	pthread_t msg_pool_id;
	pthread_create(&msg_pool_id, NULL, handleserver_msg_pool_loop, session_server);
	udp_recv_message(session_server->socket_fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}



