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



typedef enum _shutdown 
{
    SHUTDOWN_READ       = 0,	
    SHUTDOWN_WRITE      = 1,	
    SHUTDOWN_READ_WRITE = 2		
} shutdown_t;





#endif /*_COMMON_H*/
