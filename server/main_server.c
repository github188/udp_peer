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

typedef struct server_session
{
	int server_socket_fd;
	struct sockaddr_in	servaddr;
	server_pthread_msg_t * recv_msg;
	server_pthread_msg_t * send_msg;
	
}server_session_t;


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

	session_server->server_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(session_server->server_socket_fd < 0)
	{
		dbg_printf("socket fail \n");
		goto fail;
	}

	bzero(&session_server->servaddr, sizeof(struct sockaddr_in));
	session_server->servaddr.sin_family      = AF_INET;
	session_server->servaddr.sin_addr.s_addr = udp_get_localaddres(NET_INTERFACE);
	session_server->servaddr.sin_port        = htons(SERVER_PORT);

	setsockopt(session_server->server_socket_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	setsockopt(session_server->server_socket_fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	ret = bind(session_server->server_socket_fd, (struct sockaddr *) &session_server->servaddr, sizeof(struct sockaddr));
	if(ret != 0 )
	{
		dbg_printf("bing fail \n");
		goto fail;
	}

	session_server->recv_msg = servermsg_new(MSG_MAX_NUM);
	if(NULL == session_server->recv_msg)
	{
		dbg_printf("servermsg_new fail \n");
		goto fail;
	}

	session_server->send_msg = servermsg_new(MSG_MAX_NUM);
	if(NULL == session_server->send_msg)
	{
		dbg_printf("servermsg_new fail \n");
		goto fail;
	}

	
	return(0);

fail:



	if(session_server->server_socket_fd > 0)
	{
		udp_shutdown_socket(session_server->server_socket_fd,SHUTDOWN_READ_WRITE);
	}

	if(NULL != session_server)
	{
		free(session_server);
		session_server = NULL;
	}
	return(-3);
}



static void* recvmsg_pthread_loop(void *arg)
{

	int ret = -1;
	void * task = NULL;
	server_session_t * server = (server_session_t *)arg;
	if(NULL == server || NULL == server->recv_msg)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
    while (1)
    {
        pthread_mutex_lock(&(server->recv_msg->mutex));
 
        while (0 == server->recv_msg->task_num)
        {
            pthread_cond_wait(&(server->recv_msg->cond), &(server->recv_msg->mutex));
        }

        ret = ring_queue_pop(&(server->recv_msg->queue), &task);
        pthread_mutex_unlock(&(server->recv_msg->mutex));
        if (ret  != 0)continue;

        dbg_printf("thread [%ld] is starting to work\n", udp_get_pid());

		
        volatile unsigned int *task_num = &(server->recv_msg->task_num);
        fetch_and_sub(task_num, 1);  
		if(NULL != task)
		{
			msg_header_t * msg = (msg_header_t *)task;
			if(LOGIN_MSG == msg->type)
			{
				char * pip_str = udp_sock_ntop(&msg->src_addr);
				dbg_printf("receive data from %s  port: %d \n",pip_str,ntohs(udp_get_port(&msg->src_addr)));
				if(NULL != pip_str)free(pip_str);	

				msg_header_t * ask_msg = calloc(1,sizeof(msg_header_t));
				if(NULL != ask_msg)
				{
					ask_msg->type = LOGIN_ASK_MSG;
					memmove(&ask_msg->dst_addr,&msg->src_addr,sizeof(struct sockaddr));
					servermsg_add(session_server->send_msg,ask_msg);
				}

				

			}
			
			free(task);
			task = NULL;

		}
		
    }

    pthread_exit(NULL);
    return NULL;
}


/*建立一条重发队列，要重发的数据放置在该队列中，而该函数每次都要检测是否有需要重发的数据，如果有，则有限响应该队列*/
static void* sendmsg_pthread_loop(void *arg)
{

	int ret = -1;
	void * task = NULL;
	server_session_t * server = (server_session_t *)arg;
	if(NULL == server || NULL == server->send_msg)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
    while (1)
    {
        pthread_mutex_lock(&(server->send_msg->mutex));
 
        while (0 == server->send_msg->task_num)
        {
            pthread_cond_wait(&(server->send_msg->cond), &(server->send_msg->mutex));
        }

        ret = ring_queue_pop(&(server->send_msg->queue), &task);
        pthread_mutex_unlock(&(server->send_msg->mutex));
        if (ret  != 0)continue;

        dbg_printf("send thread [%ld] is starting to work\n", udp_get_pid());

		
        volatile unsigned int *task_num = &(server->send_msg->task_num);
        fetch_and_sub(task_num, 1);  
		if(NULL != task)
		{
			msg_header_t * msg = (msg_header_t * )task;
			sendto(server->server_socket_fd, msg, sizeof(msg_header_t), 0, &(msg->dst_addr),sizeof(struct sockaddr));

			
			free(task);
			task = NULL;

		}
		
    }

    pthread_exit(NULL);
    return NULL;
}




void dg_echo(int sockfd, struct sockaddr *pcliaddr, socklen_t clilen)
{
	int			n;
	socklen_t	len;
	char		mesg[MAXLINE];

	for ( ; ; ) 
	{
		len = clilen;
		n = recvfrom(sockfd, mesg, MAXLINE, 0, pcliaddr, &len);
		if(n <= 0 )continue;
		
		void * data = calloc(1,sizeof(unsigned char)*(n+1));
		if(NULL == data)continue;
		if(n < sizeof(msg_header_t))continue;
		
		memmove(data,mesg,n);
		servermsg_add(session_server->recv_msg,data);
		
		
		#if 0
		char * pip_str = udp_sock_ntop(pcliaddr);
		dbg_printf("receive data from %s  port: %d \n",pip_str,ntohs(udp_get_port(pcliaddr)));
		if(NULL != pip_str)free(pip_str);
		sendto(sockfd, mesg, n, 0, pcliaddr, len);
		#endif

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

	pthread_t recvmsg_id;
	pthread_t sendmsg_id;
	pthread_create(&recvmsg_id, NULL, recvmsg_pthread_loop, session_server);
	pthread_create(&sendmsg_id, NULL, sendmsg_pthread_loop, session_server);
	dg_echo(session_server->server_socket_fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}



