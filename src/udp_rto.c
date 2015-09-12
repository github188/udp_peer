#include "common.h"
#include "udp_rto.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"udp_rto:"





rto_info_t * rto_new(void)
{
	int i = 0;
	rto_info_t * rto_temp = NULL;
	rto_temp = calloc(1,sizeof(rto_info_t));
	if(NULL == rto_temp)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}

	for(i=0;i<SAMPLE_NUM; ++ i)
	{
		rto_temp->sample_data[i] = DEFAULT_RTO_VALUE;
	}

	rto_temp->index = 0;
	
	return(rto_temp);

}


int rto_push_sample_data(rto_info_t * info,unsigned int data)
{
	if(NULL == info)
	{
		dbg_printf("this is null \n");
		return(-1);
	}
	info->sample_data[info->index] = data;
	info->index = (info->index+1)%(SAMPLE_NUM+1);
	return(0);
}


unsigned int rto_get(rto_info_t * info)
{
	int i = 0;
	unsigned int sum = 0;
	if(NULL == info)
	{
		dbg_printf("check the param \n");
		return(DEFAULT_RTO_VALUE);
	}

	for(i=0;i<SAMPLE_NUM;++i)
	{
		sum += info->sample_data[i];
	}

	return(2*((unsigned int)(sum/SAMPLE_NUM)));
}












