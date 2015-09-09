#include "common.h"
#include "udp_servermsg.h"



server_pthread_msg_t * servermsg_new(unsigned int msg_num)
{
	int ret = -1;
	server_pthread_msg_t *server_msg = NULL;

	if(msg_num == 0 )
	{
		dbg_printf("check the param \n");
		return(NULL);
	}
	server_msg = calloc(1,sizeof(server_pthread_msg_t));
	if(NULL == server_msg)
	{
		dbg_printf("calloc fail \n");
		return(NULL);
	}

	
	ret = ring_queue_init(&server_msg->handle_msg_queue, msg_num);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		goto fail;
	}

	ret = ring_queue_init(&server_msg->send_msg_queue, msg_num);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		goto fail;
	}

	ret = ring_queue_init(&server_msg->resend_msg_queue, msg_num);
	if(ret < 0 )
	{
		dbg_printf("ring_queue_init  fail \n");
		goto fail;
	}

	
    pthread_mutex_init(&(server_msg->mutex), NULL);
    pthread_cond_init(&(server_msg->cond), NULL);
	server_msg->handle_msg_num = 0;
	server_msg->send_msg_num = 0;
	server_msg->resend_msg_num = 0;

	return(server_msg);


fail:

	if(NULL != server_msg)
	{
		free(server_msg);
		server_msg = NULL;
	}

	return(NULL);
	
}


int servermsg_add_handle_msg(server_pthread_msg_t *server_msg, void *task)
{
    int i, ret;
	server_pthread_msg_t *server = server_msg;
	if(NULL == server || NULL == task)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
    pthread_mutex_lock(&(server->mutex));

    for (i = 0; i < 1000; i++)
    {
        ret = ring_queue_push(&server->handle_msg_queue, task);
        if (ret < 0)
        {
            usleep(i);
            continue;
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(server->mutex));

    if (ret < 0)
    {
        return (-1);
    }
    else
    {
        volatile unsigned int *task_num = &server->handle_msg_num;
        fetch_and_add(task_num, 1);
    }
    return pthread_cond_signal(&(server->cond));
}



int servermsg_add_send_msg(server_pthread_msg_t *server_msg, void *task)
{
    int i, ret;
	server_pthread_msg_t *server = server_msg;
	if(NULL == server || NULL == task)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
    pthread_mutex_lock(&(server->mutex));

    for (i = 0; i < 1000; i++)
    {
        ret = ring_queue_push(&server->send_msg_queue, task);
        if (ret < 0)
        {
            usleep(i);
            continue;
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(server->mutex));

    if (ret < 0)
    {
        return (-1);
    }
    else
    {
        volatile unsigned int *task_num = &server->send_msg_num;
        fetch_and_add(task_num, 1);
    }
    return pthread_cond_signal(&(server->cond));
}



int servermsg_add_resend_msg(server_pthread_msg_t *server_msg, void *task)
{
    int i, ret;
	server_pthread_msg_t *server = server_msg;
	if(NULL == server || NULL == task)
	{
		dbg_printf("check the param \n");
		return(-1);
	}
    pthread_mutex_lock(&(server->mutex));

    for (i = 0; i < 1000; i++)
    {
        ret = ring_queue_push(&server->resend_msg_queue, task);
        if (ret < 0)
        {
            usleep(i);
            continue;
        }
        else
        {
            break;
        }
    }
    pthread_mutex_unlock(&(server->mutex));

    if (ret < 0)
    {
        return (-1);
    }
    else
    {
        volatile unsigned int *task_num = &server->resend_msg_num;
        fetch_and_add(task_num, 1);
    }
    return pthread_cond_signal(&(server->cond));
}



