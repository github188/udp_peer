###功能
去定时器化的udp可靠传输实现。

###实现原理
tcp的可靠性，在一定意义上依赖于其重传、纠错、带宽控制等机制，udp在这方面是缺乏的，不过正因为udp不具有这些特性，在大数据的传输上，udp在速度上很具有优势。特别是在某些需要网络数据透传的地方，也会采用udp协议进行“打洞”。毋庸置疑，当用udp传输某些关键数据时，我们也希望udp具有一定的可靠性，通常，可靠udp的实现方式是开启一个定时器，定时进行检测，当预期的数据未到达时，进行重传，这种loop模式的检测是很耗资源的，特别是在嵌入式环境中，这不是一个很好的选择。我们希望recvfrom是react模型的，一切的触发都来自外界，我们需要做的仅仅是对外界的触发做出反应，而不用主动的采用类似loop模型的去主动检测事件。下面是实现的关键点：

1、套接字设置为非阻塞

2、创建一个wakeup_fd，它主要用来abort等待

3、把套接字和eventfd同时放入监控：

```
fd_set			rset;
struct timeval	tv;
FD_ZERO(&rset);
FD_SET(client->socket_fd, &rset);
FD_SET(client->wakeup_fd, &rset);
```

4、对于可靠的udp包，放入reliable_queue中

5、每次recvform或timeout时，检测reliable_queue中可靠包的数目，如果为0，进入完全阻塞状态：
```
ret = select(maxfd+1, &rset, NULL, NULL,NULL);
```
如果不为0，计算rtto，执行：
```
			tv.tv_sec = 0;
			tv.tv_usec = rto_time_out * 1000;
			ret = select(maxfd+1, &rset, NULL, NULL, &tv);
```
当你需要发送可靠包或者希望从阻塞中唤醒时，执行

```
    udp_wakeup_send(client->wakeup_fd)

```

6、关于rtto的计算
这里采用的是简单的平均值实现方式，如果擅长karn所提出的那种rto计算方式，可以尝试一下。


###LICENCE
MIT

###OTHERS
jweihsz@qq.com
