#include "common.h"
#include "udp_wakeup.h"
#include <sys/eventfd.h>

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"udp_wakeup:"



int udp_wakeup_new(void)
{
	int evnet_fd = eventfd(0, EFD_NONBLOCK);
	if(evnet_fd < 0 )
	{
		dbg_printf("eventfd is fail  ! \n");
		return(-1);
	}
	return(evnet_fd);
}




int udp_wakeup_send(int fd)
{

	uint64_t value = 1;
	int nbytes = 0;
	if(fd <= 0)
	{
		dbg_printf("check the param \n");
		return(-1);
	}

    while (1)
    {
        nbytes = write(fd, &value, sizeof(value));
        if (nbytes > 0)
        {
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                usleep(1000);
                continue;
            }
            else
            {
                break;
            }
        }
    }

	return(nbytes);

}




int udp_wakeup_clean(int fd)
{

	uint64_t value = 0;
	int ret = -1;
	if(fd < 0)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
	/*no block mode */
	do
	{
		ret = read(fd,&value,sizeof(value));
	}while(value !=0 && ret>0);


	return(value);

}