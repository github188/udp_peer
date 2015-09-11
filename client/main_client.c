#include "common.h"
#include "udp_basemsg.h"
#include "udp_servermsg.h"
#include "udp_rto.h"
#include "udp_packets.h"
#include "udp_wakeup.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"main_client:"


#undef MAXLINE
#define MAXLINE   1024

#define	NET_INTERFACE	"eth0"

#define	MSG_MAX_NUM		(1024)


static  rto_info_t * rtoinfo = NULL;



static  server_session_t * session_client = NULL;
volatile unsigned int rto_time_out = DEFAULT_RTO_VALUE;
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
	udp_fcntl_set_block(session_client->socket_fd,1);

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

	session_client->wakeup_fd = udp_wakeup_new();
	if(session_client->wakeup_fd < 0)
	{
		dbg_printf("udp_wakeup_new is fai \n");
		goto fail;
	}


	session_client->reliable_queue = udp_packets_new();
	if(NULL == session_client->reliable_queue)
	{
		dbg_printf("udp_packets_new fail \n");
		goto fail;

	}

	session_client->packets_index = 0;

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


msg_data_t * client_creat_packet(packet_list_t * list,msg_type_t type,char is_reliable,void * data,unsigned length)
{
	if(0 == is_reliable )
	{
		msg_data_t * msgdata = (msg_data_t * )calloc(1,sizeof(msg_data_t));
		msgdata->type = type;
		msgdata->data_length = length;
		msgdata->is_reliable = is_reliable;
		memmove(&(msgdata->src_addr),&clisent_addr,sizeof(struct sockaddr_in));
		if(NULL != data  && length != 0)
		{
			msgdata->data = calloc(1,length+1);
			memmove(msgdata->data,data,length);
		}
		return(msgdata);
	}
	else
	{

		if(NULL == list)
		{
			return(NULL);
		}
		reliable_packet_t * packet = calloc(1,sizeof(reliable_packet_t));
		packet->msg.type = type;
		packet->msg.data_length = length;
		packet->msg.is_reliable =is_reliable; 
		memmove(&(packet->msg.src_addr),&clisent_addr,sizeof(struct sockaddr_in));
		if(NULL != data && length != 0)
		{
			packet->msg.data = calloc(1,length+1);
			memmove(packet->msg.data,data,length);
		}

		packet_node_t * node_packet = calloc(1,sizeof(packet_node_t));
		if(NULL == node_packet)
		{
			dbg_printf("calloc is null \n");
			free(packet);
			packet = NULL;
			return(NULL);
		}
		node_packet->packet = packet;
		udp_push_packets(list,node_packet);
		
		return(&packet->msg);
	}
	return(NULL);

}



int clientmsg_handle_login(server_session_t * session,void * data)
{


	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}

	char temp_data[1440];
	int byte_lenrth = 0;
	msg_data_t * msgdata = (msg_data_t * )data;
	
	msgdata->packer_index = udp_get_packet_index(&session->packets_index);
	if(msgdata->is_reliable)
	{
		reliable_packet_t * reliable = 	container_of(msgdata,reliable_packet_t,msg);
		reliable->retry_times = 0;
		reliable->time_stamp_start = udp_get_curtime();
		reliable->time_stamp_end = reliable->time_stamp_start +rto_get(rtoinfo);
	}

	memmove(temp_data,msgdata,sizeof(msg_data_t));
	
	if(msgdata->data_length)
	{
		memmove(&temp_data[sizeof(msg_data_t)],msgdata->data,msgdata->data_length);	
	}
	byte_lenrth = sizeof(msg_data_t) + msgdata->data_length;
	write(session->socket_fd, temp_data, byte_lenrth);

	if(0 == rto_time_out)
	{
		udp_wakeup_send(session->wakeup_fd);
	}
	
	

	if(0 == msgdata->is_reliable)
	{
		if( NULL != msgdata->data)
		{
			free(msgdata->data);
			msgdata->data = NULL;
		}

		if(NULL != msgdata)
		{
			free(msgdata);
			msgdata = NULL;
			
		}
	}

	return(0);

}

int clientmsg_handle_login_ask(server_session_t * session,void * data)
{
	if(NULL == session || NULL == data)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	msg_data_t * msg = (msg_data_t *)data;
	if(LOGIN_ASK_MSG == msg->type)
	{
		dbg_printf("recv the ask ,msg->time_stamp ===  %ld \n",msg->packer_index);
	}

	free(data);
	data = NULL;
	


	msg_data_t * login_msg = client_creat_packet(session->reliable_queue,LOGIN_MSG,1,NULL,0);
	if(NULL != login_msg)
	{
		servermsg_add_send_msg(session->msg_pool,login_msg);
	}
	

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
			msg_data_t * msg = (msg_data_t *)task;
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






/*
思路:增加一个abort pipe，来激活select中的休眠，每次休眠完毕之后，如果暂时不需要再接收数据，设置很长的休眠时间 或者设置为-1，如果需要收数据时候，再重新设置


*/
void udp_client_run(server_session_t * client, const struct sockaddr *pservaddr, socklen_t servlen)
{
	int		n;
	int ret = -1;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];


	if(NULL == client)
	{
		dbg_printf("check the param \n");
		return;
	}


	msg_data_t * login_msg = client_creat_packet(client->reliable_queue,LOGIN_MSG,1,NULL,0);
	if(NULL != login_msg)
	{
		servermsg_add_send_msg(session_client->msg_pool,login_msg);
	}
	
	int maxfd = (client->socket_fd > client->wakeup_fd) ? (client->socket_fd) : (client->wakeup_fd);
	while(1)
	{


		fd_set			rset;
		struct timeval	tv;
		FD_ZERO(&rset);
		FD_SET(client->socket_fd, &rset);
		FD_SET(client->wakeup_fd, &rset);


		if(0 == rto_time_out)
		{
			dbg_printf("wait forever ! \n");
			ret = select(maxfd+1, &rset, NULL, NULL,NULL);
		}
		else
		{
			tv.tv_sec = 0;
			tv.tv_usec = rto_time_out * 1000;
			ret = select(maxfd+1, &rset, NULL, NULL, &tv);
		}
		rto_time_out = 0;  /*暂时*/

		if(-1 == ret)
		{
			if (errno == EINTR)continue;
			else
			{
				dbg_printf("come out \n");
			}	
		}
		else if(0 == ret )
		{
			dbg_printf("read time out \n");
			rto_time_out = rto_get(rtoinfo) * 2;
			continue;
		}

		if(FD_ISSET(client->wakeup_fd,&rset))
		{
			dbg_printf("just wake up ! \n");
			udp_wakeup_clean(client->wakeup_fd);
			rto_time_out = rto_get(rtoinfo);

			dbg_printf("rto_time_out==%ld \n",rto_time_out);
			continue;
		}
		


		n = read(client->socket_fd, recvline, MAXLINE);
		if(n < sizeof(msg_data_t))
		{
			dbg_printf("read time wrong \n");
			continue;
		}

		msg_data_t * msg = (msg_data_t *)recvline;
		
		if(1 == msg->is_reliable)
		{
			
			unsigned int diff_time;
			diff_time = udp_find_and_delpackets(client->reliable_queue,msg->packer_index);
			if(diff_time > 0)
			{
				rto_push_sample_data(rtoinfo,diff_time);
				
			}
			dbg_printf("the diff time is %u \n",diff_time);
			
		
			
		}
			
		if(LOGIN_ASK_MSG == msg->type)
		{
			msg_data_t * msg_recv = calloc(1,sizeof(unsigned char)*(n+1));
			memmove(msg_recv,recvline,n);
			servermsg_add_handle_msg(session_client->msg_pool,msg_recv);
			
		}


	}

}




int main(int argc, char **argv)
{


	struct sockaddr_in	servaddr;

	if (argc != 2)
		dbg_printf("usage: udpcli <IPaddress>\n");

	session_client_init(argv[1]);

	rtoinfo = rto_new();
	if(NULL == rtoinfo)
	{
		dbg_printf("rto_new fail \n");
		return(-1);
	}

	pthread_t clientmsg_pool_id;
	pthread_create(&clientmsg_pool_id, NULL, handleclient_msg_pool_loop, session_client);
	udp_client_run(session_client, (struct  sockaddr *) &session_client->servaddr, sizeof(servaddr));

	exit(0);
}



