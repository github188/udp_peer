#ifndef _udp_rto_h
#define _udp_rto_h


typedef  struct  rto_info
{

	unsigned int rtts;
	unsigned int rttd;
	unsigned int rttm;
	unsigned int rtto;

}rto_info_t;



rto_info_t * rto_new(void);
int  rto_reget_rtovalue(rto_info_t * rto_info,unsigned int rttm_value);
int rto_init(rto_info_t * rto_info);




#endif