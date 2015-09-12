#include "common.h"
#include "udp_packets.h"

#undef	DBG_ON
#undef	FILE_NAME	
#define	DBG_ON  			(0x01)
#define	FILE_NAME 			"udp_packets:"





packet_list_t * udp_packets_new(void)
{
	packet_list_t * packetlist = calloc(1,sizeof(packet_list_t));
	if(NULL == packetlist)
	{
		dbg_printf("calloc is fail \n");
		return(NULL);
	}
	pthread_mutex_init(&(packetlist->mutex), NULL);
	TAILQ_INIT(&packetlist->packet_queue);
	packetlist->num_packets = 0;
	return(packetlist);
}




int udp_get_packets_num(packet_list_t * list)
{
	
	int which = 0;
	if(list == NULL )return(-1);
	pthread_mutex_lock(&list->mutex);
	which = list->num_packets;
	pthread_mutex_unlock(&list->mutex);
	return(which);
}



int udp_push_packets(packet_list_t * list,packet_node_t * node )
{

	if(NULL == node ||  NULL == list)
	{
		dbg_printf("check the param");
		return(-1);
	}
	
	pthread_mutex_lock(&list->mutex);
	TAILQ_INSERT_TAIL(&list->packet_queue,node,links);
	list->num_packets += 1;
	pthread_mutex_unlock(&list->mutex);
	return(0);
}

packet_node_t * udp_get_first_packet(packet_list_t * list)
{

	packet_node_t * return_node = NULL;
	if(NULL == list)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}

	pthread_mutex_lock(&list->mutex);
	return_node = TAILQ_FIRST(&list->packet_queue);
	pthread_mutex_unlock(&list->mutex);
	return(return_node);

}



reliable_packet_t * udp_find_packets(packet_list_t * list,unsigned long index)
{


	if(NULL == list)
	{
		dbg_printf("check the param \n");
		return(NULL);
	}

	packet_node_t * node = NULL;
	reliable_packet_t * return_packet = NULL;
	pthread_mutex_lock(&list->mutex);
	TAILQ_FOREACH(node,&list->packet_queue,links)
	{
		if(index == node->packet->msg.packer_index)
		{
			return_packet = node->packet;
			break;
		}
	}
	pthread_mutex_unlock(&list->mutex);
	
	return(return_packet);
}


unsigned long udp_find_and_delpackets(packet_list_t * list,unsigned long index)
{


	unsigned long real_end_time = 0;
	if(NULL == list)
	{
		dbg_printf("check the param \n");
		return(0);
	}

	packet_node_t * node = NULL;
	pthread_mutex_lock(&list->mutex);
	TAILQ_FOREACH(node,&list->packet_queue,links)
	{
		if(index == node->packet->msg.packer_index)
		{
			TAILQ_REMOVE(&list->packet_queue,node,links);
			dbg_printf("i find it   %ld\n",node->packet->msg.packer_index);

			real_end_time = udp_get_curtime();
			dbg_printf("the real_end_time time is == %ld  node->packet->time_stamp_end===%ld \n",real_end_time,node->packet->time_stamp_end);
			dbg_printf("node->packet->time_stamp_start===%ld \n",node->packet->time_stamp_start);
			if(real_end_time < node->packet->time_stamp_end )
			{
				real_end_time = real_end_time-node->packet->time_stamp_start;
			}
			else
			{
				real_end_time = 0;
			}
			
			if(NULL != node->packet->msg.data)
			{
				free(node->packet->msg.data);
				node->packet->msg.data = NULL;
			}
						
			if(NULL != node->packet)
			{
				free(node->packet);
				node->packet = NULL;
			}

			if(NULL != node)
			{
				free(node);
				node = NULL;
			}

			list->num_packets -= 1;
			break;
		}
	}
	pthread_mutex_unlock(&list->mutex);

	return(real_end_time);
	
}








