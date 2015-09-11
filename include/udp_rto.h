#ifndef _udp_rto_h
#define _udp_rto_h

#define	DEFAULT_RTO_VALUE	(1*1000)
#define  SAMPLE_NUM		(10)

typedef  struct  rto_info
{
	int index;
	unsigned int sample_data[SAMPLE_NUM];
}rto_info_t;



rto_info_t * rto_new(void);
int rto_push_sample_data(rto_info_t * info,unsigned int data);
unsigned int rto_get(rto_info_t * info);




#endif