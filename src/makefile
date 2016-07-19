all: client server

client: client.c
	c99 -o client.o client.c udpsendcli.c udpgetcli.c

server: server.c
	c99 -o server.o server.c tcp.c filelist.c udpget.c udpsend.c portlist.c
