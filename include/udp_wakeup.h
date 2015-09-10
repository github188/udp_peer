#ifndef _udp_wakeup_h
#define	_udp_wakeup_h



int udp_wakeup_new(void);
int udp_wakeup_send(int fd);
int udp_wakeup_clean(int fd);


#endif /*_udp_wakeup_h*/

