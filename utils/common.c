#include "common.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"common:"



int udp_set_recvbuff_size(int socket_fd,int size )
{
	int ret = -1;
	if(socket_fd<0  || size <= 0)
	{
		dbg_printf("check the param\n");
		return(-1);
	}

	ret = setsockopt(socket_fd,SOL_SOCKET,SO_RCVBUF,&size,sizeof(size));
	if(ret != 0)return(-2);

	return(0);
}


int udp_set_sendbuff_size(int socket_fd,int size )
{
	int ret = -1;
	if(socket_fd<0  || size <= 0)
	{
		dbg_printf("check the param\n");
		return(-1);
	}

	ret = setsockopt(socket_fd,SOL_SOCKET,SO_SNDBUF,&size,sizeof(size));
	if(ret != 0)return(-2);

	return(0);
}

int udp_set_block(int socket_fd,int is_block)
{

	int value;
	int ret = -1;
	do
	{
		value = fcntl(socket_fd,F_GETFL);
	}while(value<0 && errno==EINTR);

	if(value < 0)
	{
		dbg_printf("this is wrong to get \n");
		return (-1);
	}

    if (is_block)
		value = value & ~O_NONBLOCK;   
    else
		value = value | O_NONBLOCK;
	do
	{
		ret = fcntl(socket_fd,F_SETFL,value);
		
	}while(ret <0 && errno==EINTR );

	if(ret <0)
	{
		dbg_printf("set the fanctl fail\n");
		return(-2);
	}
	return(0);
}



char * udp_sock_ntop(struct sockaddr *sa)
{

	if(NULL == sa  )
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
	#define  DATA_LENGTH	128
	
	char * str  = calloc(1,sizeof(char)*DATA_LENGTH);
	if(NULL == str)
	{
		dbg_printf("calloc is fail\n");
		return(NULL);
	}

	switch(sa->sa_family)
	{
		case AF_INET:
		{

			struct sockaddr_in * sin = (struct sockaddr_in*)sa;

			if(inet_ntop(AF_INET,&(sin->sin_addr),str,DATA_LENGTH) == NULL)
			{
				free(str);
				str = NULL;
				return(NULL);	
			}
			return(str);

		}
		case AF_INET6:
		{

			struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)sa;
			if(inet_ntop(AF_INET,&sin6->sin6_addr,str,DATA_LENGTH) == NULL )
			{
				free(str);
				str = NULL;
				return(NULL);		
			}
			return(str);
		}

		case AF_UNIX:
		{
			struct sockaddr_un	*unp = (struct sockaddr_un *) sa;
			if (unp->sun_path[0] == 0)
				strcpy(str, "(no pathname bound)");
			else
				snprintf(str, DATA_LENGTH, "%s", unp->sun_path);
			return(str);
		}
		default:
		{
			free(str);
			str = NULL;
			return(NULL);
		}


	}
			
    return (NULL);
}


int udp_sock_pton(char * net_addres, struct sockaddr *sa)
{
	if(NULL == net_addres || NULL == sa)
	{
		dbg_printf("please check the param\n");
		return(-1);
	}
	switch(sa->sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in * sin = (struct sockaddr_in*)sa;
			if(inet_pton(AF_INET,net_addres,&sin->sin_addr) != 1)
				return(-1);
			else
				return(0);
		}
		case AF_INET6:
		{
			struct sockaddr_in6 * sin6 = (struct sockaddr_in6 *)sa;
			if(inet_pton(AF_INET6,net_addres,&sin6->sin6_addr) != 1)
				return(-1);
			else
				return(0);

		}
		default:
		{
			return(-1);
		}
	}
	return(-1);

}

in_port_t udp_get_port(struct sockaddr *sa)
{
	if(NULL == sa)
	{
		dbg_printf("please check the param \n");
		return(0);
	}
	switch(sa->sa_family)
	{
		case AF_INET:
		{
			struct sockaddr_in * sin = (struct sockaddr_in*)sa;
			return(sin->sin_port);
		}
		case AF_INET6:
		{
			dbg_printf("AF_INET6 not support \n");
			return(0);

		}
		default:
		{
			dbg_printf("unknow type \n");
			return(0);
		}
	}
	return(0);


}

int  udp_sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2)			 
{

	if(NULL ==sa1 || NULL ==  sa2)return(-1);
	if (sa1->sa_family != sa2->sa_family)return(-1);
		
	switch (sa1->sa_family)
	{
		case AF_INET: 
		{
			return(memcmp( &((struct sockaddr_in *) sa1)->sin_addr,
				&((struct sockaddr_in *) sa2)->sin_addr,sizeof(struct in_addr)));				   
		}
		case AF_INET6: 
		{
			return(memcmp( &((struct sockaddr_in6 *) sa1)->sin6_addr,
				&((struct sockaddr_in6 *) sa2)->sin6_addr, sizeof(struct in6_addr)));
					   
					  
		}
		case AF_UNIX:
		{
			return(strcmp( ((struct sockaddr_un *) sa1)->sun_path,
						   ((struct sockaddr_un *) sa2)->sun_path));
		}
	}
    return (-1);
}



int udp_get_rand(int min, int max)
{

	if(min >= max)
	{
		dbg_printf("please check the pram \n");
		return(-1);
	}
	static int seed = 0;
    if (seed == 0)
    {
        seed = time(NULL);
        srand(seed);
    }

	int rand_value = (int)rand()%max;
	if(rand_value < min)
		return(min + rand_value);
	return(rand_value);

}


pthread_t udp_get_pid(void)
{
 	return(pthread_self());
}

long udp_get_file_szie(FILE * fp)
{
	if(NULL == fp)return(0);
	long pos = ftell(fp);
	fseek(fp,0L,SEEK_END);
	long size = ftell(fp);
	fseek(fp,pos,SEEK_SET);
	return(size);
}


int udp_clean_recvbuff(int socket_fd)
{
	char buff[128];
	if(socket_fd < 0)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	while(recv(socket_fd,buff,128,MSG_DONTWAIT) > 0);
	return(0);
}

/*参考:linux下socket的close和shutdown*/
int udp_shutdown_socket(int socketfd,shutdown_t way)
{
	if(socketfd < 0 )
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	shutdown (socketfd, (int)way);
	return(0);
}


int udp_close_socket(int socketfd)
{
	if(socketfd < 0 )
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	close(socketfd);
	return(0);
}

int udp_wait_writeable(int socket_fd,unsigned int delay_ms)
{
	if(socket_fd < 0 )
	{
		dbg_printf("check the param\n");
		return(-1);
	}

	int ret = -1;
    struct pollfd event;
    event.fd = socket_fd;
    event.events |= POLLOUT;
	ret = poll(&event, 1, delay_ms);
	if(ret <= 0)return(-2);

	return(0);
}


int udp_wait_readable(int socket_fd,unsigned int delay_ms)
{
	if(socket_fd < 0 )
	{
		dbg_printf("check the param\n");
		return(-1);
	}

	int ret = -1;
    struct pollfd event;
    event.fd = socket_fd;
    event.events |= POLLIN;
	ret = poll(&event, 1, delay_ms);
	if(ret <= 0)return(-2);

	return(0);
}





int udp_sendto(int socket_fd, void * data, int length, struct sockaddr *dest_addr)
{

	if(socket_fd <0  || NULL==data || length<=0  || NULL == dest_addr)
	{
		dbg_printf("check the param\n");
		return(-1);
	}

	int bytes = -1;
	while(1)
	{
		bytes = sendto(socket_fd, data, length, 0, (struct sockaddr*)dest_addr, sizeof(struct sockaddr_in));
		if(bytes < 0)
		{
			if (errno == EINTR)continue;
			else
				return(-2);
		}
		else
		{
			break;
		}
	}
	return(bytes);

}


int udp_recvfrom(int socket_fd, void * data, int length, struct sockaddr *src_addr)
{
	if(socket_fd <0  || NULL==data || length<=0  || NULL == src_addr)
	{
		dbg_printf("check the param\n");
		return(-1);
	}
	int bytes = -1;
 	socklen_t length_sock = sizeof(struct sockaddr_in);

	while(1)
	{
		bytes = recvfrom(socket_fd, data,length, 0, (struct sockaddr *)src_addr, &length_sock);
		if(bytes < 0 )
		{
			if (errno == EINTR)continue;
			else
				return(-2);
		}
		else
		{
			break;
		}   
	}
	return(bytes);
}
	


/*没有连接的UDP不能调用getpeername,但是可以调用getsockname*/
int udp_getsock_name(int sock_fd,struct sockaddr * sa)
{
	if(NULL ==sa || sock_fd <= 0 )return(-1);
	socklen_t len = sizeof(struct sockaddr);
	if(getsockname(sock_fd,sa,&len) < 0 )
	{
		return(-1);
	}
	return(0);
}

int udp_getpeer_name(int sock_fd,struct sockaddr * peer)
{
	if(NULL ==peer || sock_fd <= 0 )return(-1);
	socklen_t len = sizeof(struct sockaddr);
	if(getpeername(sock_fd,peer,&len) < 0 )
	{
		return(-1);
	}
	return(0);
}

unsigned long udp_get_localaddres(const unsigned char * net_face)
{
	int fd_socket;
	int ret = -1;
	if(NULL == net_face)
	{
		dbg_printf("check the param \n");
		return(0);
	}
    if((fd_socket=socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        dbg_printf("socket fail \n");
        return (0);
    }
    struct ifreq ifr_ip;
    memset(&ifr_ip, 0, sizeof(ifr_ip));
    strncpy(ifr_ip.ifr_name, net_face, sizeof(ifr_ip.ifr_name) - 1);
    if(ioctl(fd_socket, SIOCGIFADDR, &ifr_ip) < 0)
    {
        dbg_printf("ioctl fail \n");
        return (0);
    }
    struct sockaddr_in *sin;
    sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;
    close(fd_socket);
    return sin->sin_addr.s_addr;

}
	


	
unsigned long  udp_get_curtime(void)
{

	struct timeval time_value;
	int ret = -1;
	ret = gettimeofday(&time_value,NULL);
	if(ret < 0 )
	{
		dbg_printf("get time fail \n");
		return(0);
	}
	return(time_value.tv_sec * 1000 + time_value.tv_usec / 1000);

}


int udp_read_timeout(int fd,unsigned int mssec)
{
	if(fd < 0)
	{
		dbg_printf("please check the socket\n");
		return(-1);
	}
	fd_set			rset;
	struct timeval	tv;
	FD_ZERO(&rset);
	FD_SET(fd, &rset);
	tv.tv_sec = 0;
	tv.tv_usec = mssec * 1000;
	return(select(fd+1, &rset, NULL, NULL, &tv));
 
}


unsigned long udp_get_packet_index(unsigned long * value)
{

	unsigned long return_value = 0;
	compare_and_swap(value,65535,0);
	fetch_and_add(value,1);
	return_value = *value;
	return(return_value);
	
}



int udp_fcntl_set_block(int sock,int nonblock)
{

	int value;
	int ret = -1;
	do
	{
		value = fcntl(sock,F_GETFL);
	}while(value<0 && errno==EINTR);

	if(value < 0)
	{
		dbg_printf("this is wrong to get \n");
		return (-1);
	}

    if (nonblock)
    {
        value = value | O_NONBLOCK;
    }
    else
    {
        value = value & ~O_NONBLOCK;
    }

	do
	{
		ret = fcntl(sock,F_SETFL,value);
		
	}while(ret <0 && errno==EINTR );

	if(ret <0)
	{
		dbg_printf("set the fanctl fail\n");
		return(-2);
	}
	return(0);


}





