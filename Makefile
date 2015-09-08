
serverbin:
	gcc -o  2server_bin  -I./utils   -I./include ./utils/common.c  ./src/udp_ringqueue.c  ./src/udp_servermsg.c  ./server/main_server.c     -lpthread

clientbin:
	gcc -o  1client_bin  -I./utils  -I./include ./utils/common.c  ./src/udp_ringqueue.c ./src/udp_servermsg.c  ./client/main_client.c     -lpthread

cc:
	-rm -rf  1client_bin   2server_bin









