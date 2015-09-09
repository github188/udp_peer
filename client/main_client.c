#include "common.h"
#include "udp_basemsg.h"
#include "udp_servermsg.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"main_client:"


#undef MAXLINE
#define MAXLINE   1024

#define	NET_INTERFACE	"eth0"

#define	MSG_MAX_NUM		(1024)


static  server_session_t * session_client = NULL;

struct sockaddr_in	clisent_addr;

int session_client_init( char * server_addres)
{

	int ret = -1;
	int size = 220 * 1024;
	if(NULL != session_client)
	{
		dbg_printf("init already!\n");
		return(-1);
	}

	if(NULL == server_addres)
	{
		dbg_printf("please input the server ip addres !\n");
		return(-1);
	}
	session_client = calloc(1,sizeof(server_session_t));
	if(NULL == session_client)
	{
		dbg_printf("calloc fail \n");
		return(-2);
	}


	bzero(&session_client->servaddr, sizeof(struct sockaddr_in));
	session_client->servaddr.sin_family      = AF_INET;
	session_client->servaddr.sin_port        = htons(45620);
	udp_sock_pton(server_addres,(struct  sockaddr *)&session_client->servaddr);
	
	session_client->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(session_client->socket_fd < 0)
	{
		dbg_printf("socket fail \n");
		goto fail;
	}

	setsockopt(session_client->socket_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	setsockopt(session_client->socket_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));


	bzero(&clisent_addr, sizeof(clisent_addr));
	clisent_addr.sin_family = AF_INET;
	clisent_addr.sin_addr.s_addr = udp_get_localaddres(NET_INTERFACE);
	clisent_addr.sin_port = htons(45625);
	ret = bind(session_client->socket_fd, (struct sockaddr *) &clisent_addr, sizeof(struct sockaddr));
	if(ret != 0 )
	{
		dbg_printf("bing fail \n");
		goto fail;
	}

	connect(session_client->socket_fd, (struct sockaddr *) &session_client->servaddr, sizeof(struct sockaddr_in));

	session_client->msg_pool = servermsg_new(MSG_MAX_NUM);
	if(NULL == session_client->msg_pool)
	{
		dbg_printf("servermsg_new fail \n");
		goto fail;
	}

	return(0);

fail:



	if(session_client->socket_fd > 0)
	{
		udp_shutdown_socket(session_client->socket_fd,SHUTDOWN_READ_WRITE);
	}

	if(NULL != session_client)
	{
		free(session_client);
		session_client = NULL;
	}
	return(-3);
}


int clientmsg_handle_login(server_session_t * session,void * data)
{
	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	write(session->socket_fd, data, sizeof(msg_header_t));

	free(data);
	data = NULL;


	return(0);

}

int clientmsg_handle_login_ask(server_session_t * session,void * data)
{
	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	msg_header_t * msg = (msg_header_t *)data;
	if(LOGIN_ASK_MSG == msg->type)
	{
		dbg_printf("recv the ask \n");
		
	}

	free(data);
	data = NULL;

	msg_header_t * login_msg = NULL;
	login_msg = calloc(1,sizeof(msg_header_t));
	if(NULL == login_msg)return;

	login_msg->type = LOGIN_MSG;
	memmove(&(login_msg->src_addr),&clisent_addr,sizeof(struct sockaddr_in));
	servermsg_add_send_msg(session_client->msg_pool,login_msg);
	
	

	return(0);

}



static handle_msg_fun_t  clientmsg_fun[]=\
{
	{LOGIN_MSG,clientmsg_handle_login},
	{LOGIN_ASK_MSG,clientmsg_handle_login_ask},
};


static void* handleclient_msg_pool_loop(void *arg)
{

	int ret = -1;
	int i = 0;
	void * task = NULL;
	server_session_t * client = (server_session_t *)arg;
	if(NULL == client || NULL == client->msg_pool)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
    while (1)
    {
        pthread_mutex_lock(&(client->msg_pool->mutex));
 
        while ((0 == client->msg_pool->handle_msg_num)&&(0 == client->msg_pool->send_msg_num)&&(0 == client->msg_pool->resend_msg_num))
        {
            pthread_cond_wait(&(client->msg_pool->cond), &(client->msg_pool->mutex));
        }

		if(0 != client->msg_pool->resend_msg_num)
		{

			ret = ring_queue_pop(&(client->msg_pool->resend_msg_queue), &task);
			pthread_mutex_unlock(&(client->msg_pool->mutex));
			if(ret != 0)continue;

			
			
	        volatile unsigned int *resend_num = &(client->msg_pool->resend_msg_num);
    		fetch_and_sub(resend_num, 1);  

			
		}
		else if(0 != client->msg_pool->handle_msg_num)
		{

			ret = ring_queue_pop(&(client->msg_pool->handle_msg_queue), &task);
			pthread_mutex_unlock(&(client->msg_pool->mutex));
			if(ret != 0)continue;
						
	        volatile unsigned int *handle_num = &(client->msg_pool->handle_msg_num);
    		fetch_and_sub(handle_num, 1);  

		}
		else if(0 != client->msg_pool->send_msg_num)
		{

			ret = ring_queue_pop(&(client->msg_pool->send_msg_queue), &task);
			pthread_mutex_unlock(&(client->msg_pool->mutex));
			if(ret != 0)continue;
			
	        volatile unsigned int *send_num = &(client->msg_pool->send_msg_num);
    		fetch_and_sub(send_num, 1);  
		}
		else
		{
			pthread_mutex_unlock(&(client->msg_pool->mutex));
			continue;

		}

		
		if(NULL != task)
		{
			msg_header_t * msg = (msg_header_t *)task;
			int find_fun  = 0;
			for(i=0;i<sizeof(clientmsg_fun)/sizeof(clientmsg_fun[0]);++i)
			{
				if(msg->type == clientmsg_fun[i].type)
				{
					find_fun = 1;
					clientmsg_fun[i].handle(client,msg);
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







void udp_client_run( int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int		n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];
	msg_header_t * login_msg = NULL;
	login_msg = calloc(1,sizeof(msg_header_t));
	if(NULL == login_msg)return;

	login_msg->type = LOGIN_MSG;
	memmove(&(login_msg->src_addr),&clisent_addr,sizeof(struct sockaddr_in));
	servermsg_add_send_msg(session_client->msg_pool,login_msg);

	while(1)
	{

		n = read(sockfd, recvline, MAXLINE);
		if(n < sizeof(msg_header_t))continue;
		
		msg_header_t * msg = (msg_header_t *)recvline;
		if(LOGIN_ASK_MSG == msg->type)
		{
			msg_header_t * msg_recv = calloc(1,sizeof(unsigned char)*(n+1));
			memmove(msg_recv,recvline,n);
			servermsg_add_handle_msg(session_client->msg_pool,msg_recv);
			
		}

	
		sleep(1);
	}

}




int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		dbg_printf("usage: udpcli <IPaddress>\n");

	session_client_init(argv[1]);
	pthread_t clientmsg_pool_id;
	pthread_create(&clientmsg_pool_id, NULL, handleclient_msg_pool_loop, session_client);
	udp_client_run(session_client->socket_fd, (struct  sockaddr *) &session_client->servaddr, sizeof(servaddr));

	exit(0);
}
