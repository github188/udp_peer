#ifndef _COMMON_H
#define _COMMON_H


#include 	<stdio.h>
#include 	<stdlib.h>
#include	<unistd.h>
#include 	<sys/stat.h>
#include 	<sys/types.h>

#include	<sys/types.h>	
#include	<sys/socket.h>	
#include	<sys/time.h>	
#include	<time.h>		
#include	<net/if.h>
#include	<sys/time.h>	
#include	<netinet/in.h>	
#include	<arpa/inet.h>	
#include	<errno.h>
#include	<fcntl.h>		
#include	<netdb.h>
#include	<signal.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>	
#include	<sys/uio.h>		
#include	<unistd.h>
#include	<sys/wait.h>
#include	<sys/un.h>		
#include	<sys/select.h>	
#include	<sys/sysctl.h>
#include	<poll.h>	
#include	<strings.h>		
#include	<sys/ioctl.h>
#include	<pthread.h>
#include 	<sys/resource.h>

#undef TRUE
#undef FALSE
#define	TRUE	(1u)
#define	FALSE	(0u)

#define DBG_ON  		(0x01)
#define FILE_NAME 	"common:"


#define dbg_printf(fmt,arg...)		do{if(DBG_ON)fprintf(stderr,FILE_NAME"%s(line=%d)->"fmt,__FUNCTION__,__LINE__,##arg);}while(0)


#define  compare_and_swap(lock,old,set)		__sync_bool_compare_and_swap(lock,old,set)
#define  fetch_and_add(value,add)			__sync_fetch_and_add(value,add)
#define	 fetch_and_sub(value,sub)			__sync_fetch_and_sub(value,sub)	

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER) 
#define  container_of(ptr, type, member)	 ({const typeof( ((type *)0)->member ) *__mptr = (ptr); (type *)( (char *)__mptr - offsetof(type,member) );})

typedef enum _shutdown 
{
    SHUTDOWN_READ       = 0,	
    SHUTDOWN_WRITE      = 1,	
    SHUTDOWN_READ_WRITE = 2		
} shutdown_t;





int udp_sock_pton(char * net_addres, struct sockaddr *sa);
char * udp_sock_ntop(struct sockaddr *sa);
in_port_t udp_get_port(struct sockaddr *sa);
int  udp_sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2)	;
pthread_t udp_get_pid(void);
int udp_shutdown_socket(int socketfd,shutdown_t way);
unsigned long udp_get_localaddres(const unsigned char * net_face);
unsigned long  udp_get_curtime(void);
int udp_read_timeout(int fd,unsigned int mssec);
int udp_fcntl_set_block(int sock,int nonblock);


#endif /*_COMMON_H*/
