#include "common.h"
#include "udp_rto.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"udp_rto:"



#define	MAX_RTO_VALUE	(2*1000)
#define	DEFAULT_RTO_VALUE	(1*1000)
#define	MIN_RTO_VALUE	(4) 


rto_info_t * rto_new(void)
{
	rto_info_t * rto_temp = NULL;
	rto_temp = calloc(1,sizeof(rto_info_t));
	if(NULL == rto_temp)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
	rto_temp->rttm = MAX_RTO_VALUE;
	rto_temp->rttd =  rto_temp->rttm / 2;
	return(rto_temp);

}


int rto_init(rto_info_t * rto_info)
{
	if(NULL == rto_info)
	{
		dbg_printf("rto_init fail \n");
		return(-1);
	}
	rto_info->rttm = MAX_RTO_VALUE;
	rto_info->rttd =  rto_info->rttm / 2;

	return(0);
}


int  rto_reget_rtovalue(rto_info_t * rto_info,unsigned int rttm_value)
{

	unsigned int diff_value = 0;
	volatile unsigned int rto = 0;
	if(NULL == rto_info)
	{
		dbg_printf("check the  param \n");
		return(-1);
	}

	rto_info->rttm = rttm_value;
	if(rto_info->rttm > rto_info->rtts)
	{
		diff_value = rto_info->rttm-rto_info->rtts;
	}
	else
	{
		diff_value = rto_info->rtts - rto_info->rttm;
	}
	rto_info->rtts = (unsigned int)((1-1/8)*rto_info->rtts + (1/8) * rto_info->rttm);
	rto_info->rttd = (unsigned int)((1-1/4)*rto_info->rttd + (1/4) * diff_value);
	rto_info->rtto = rto_info->rtts + 4 * rto_info->rttd;
	return(0);
}





