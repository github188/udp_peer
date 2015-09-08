#include "common.h"
#include "udp_servermsg.h"

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
	server_pthread_msg_t * server_msg;
	
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

	session_server->server_msg = servermsg_new(MSG_MAX_NUM);
	if(NULL == session_server->server_msg)
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



static void* msg_pthread_loop(void *arg)
{

	int ret = -1;
	void * task = NULL;
	server_session_t * server = (server_session_t *)arg;

    while (1)
    {
        pthread_mutex_lock(&(server->server_msg->mutex));
 
        while (0 == server->server_msg->task_num)
        {
            pthread_cond_wait(&(server->server_msg->cond), &(server->server_msg->mutex));
        }

        ret = ring_queue_pop(&(server->server_msg->queue), &task);
        pthread_mutex_unlock(&(server->server_msg->mutex));
        if (ret  != 0)continue;

        dbg_printf("thread [%ld] is starting to work\n", udp_get_pid());

		
        volatile unsigned int *task_num = &(server->server_msg->task_num);
        fetch_and_sub(task_num, 1);  
		if(NULL != task)
		{
			dbg_printf("the data is %s \n",(unsigned char *)task);
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
		memmove(data,mesg,n);
		servermsg_add(session_server->server_msg,data);
		
		
		#if 1
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

	pthread_t msg_id;
	pthread_create(&msg_id, NULL, msg_pthread_loop, session_server);
	dg_echo(session_server->server_socket_fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr));
}


