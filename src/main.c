/* test server, just for integer */


#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include "game.h"

#define PORT_NUM 0x401 /*1025*/

struct ClientNode {
	int fd;
	struct ClientNode * next;
	char * buff;
	int buffsize;
	int cmdlen;
};

/*ACHTUNG!!!!!!!!!! GLOBAL VARIABLE!!!!!*/
int counter = 0;

void itoa(int n, char * buff)
{
	sprintf(buff, "%i", n);
}



void SendMessage(const char * msg, struct ClientNode * cl, char finish)
{
	int i = 0;
	char newMsg[256];
	while(*msg) {
		newMsg[i] = *msg;
		msg++;
		i++;
	}
	if(finish) {
		newMsg[i] = '\n';
		i++;
	}
	write(cl->fd, newMsg, i);
}

int ExecuteCommand(const char * cmd, struct ClientNode * client)
{
	if(!strcmp(cmd, "+")) {
		counter++;
		SendMessage("counter incremented", client, 1);
	} 
	else if(!strcmp(cmd, "-")) {
		counter--;
		SendMessage("counter decremented", client, 1);
	} 
	else if(!strcmp(cmd, "print")) {
		char c_str[50];
		itoa(counter, c_str);
		SendMessage("Counter = ", client, 0);
		SendMessage(c_str, client, 1);
	} else 
		SendMessage("unknown command, try \"+\", \"-\" or \"print\"", client, 1);

		
	printf("%s\n", cmd);
	return 0;
}

/* Creates a new listening socket */
int CreateSocket(int port)
{
	int ls;
	int opt = 1;
	struct sockaddr_in addr;
	ls = socket(AF_INET,SOCK_STREAM,0);
	if(ls == -1) {
		perror("socket");
		return -1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(0 != bind(ls, (struct sockaddr *) &addr, sizeof(addr))) {
		perror("bind");
		return -1;
	}
	if(-1 == listen(ls, 5)) {
		perror("listen");
		return -1;
	}
	return ls;
}

/*Adds new client to the client list*/
struct ClientNode * AddClient(int lsock, struct ClientNode * list) 
{
	struct ClientNode * newcl = (struct ClientNode *)malloc(sizeof(*newcl));
	newcl->cmdlen = 0;
	newcl->fd = accept(lsock, NULL, NULL);
	if (newcl->fd == -1) {
		perror("accept");
		free(newcl);
		return list;
	}
	newcl->buffsize = 256;
	newcl->buff = (char *)malloc(sizeof(char)*newcl->buffsize);
	newcl->next = list;
	return newcl;
}


struct ClientNode * DeleteClient(struct ClientNode * cl, 
				struct ClientNode * prev, struct ClientNode ** clients)
{
	printf("client disconnected\n");
	if(cl == prev) {
		*clients = (*clients)->next;
		free(cl->buff);
		free(cl);
		return *clients;
	} else {
		prev->next = cl->next;
		free(cl->buff);
		free(cl);
		return prev;
	}
}
			


int ProcessClient(struct ClientNode * cl)
{
	int rsym, i;
	
	rsym = read(cl->fd, cl->buff+cl->cmdlen, cl->buffsize);
	if(rsym==0) {
		shutdown(cl->fd, 2);
		close(cl->fd);
		return 1;
	}
	if(rsym<0) {
		perror("read");
		cl->cmdlen = 0;
		return 0;
	}
	cl->buffsize+=rsym;
	cl->buff = realloc(cl->buff, cl->buffsize);
	for(i = 0; i<rsym; i++) { 
		if((cl->buff)[cl->cmdlen+i]=='\r' || (cl->buff)[cl->cmdlen+i]=='\n') {
			(cl->buff)[cl->cmdlen+i] = '\0';
			ExecuteCommand(cl->buff, cl);
			cl->cmdlen = 0;
			return 0;
		}
	}
	cl->cmdlen+=rsym;
	return 0;
}


int main(int argc, char ** argv)
{
	int lsock;
	struct ClientNode * clients = NULL;
	int port;
	if(argc == 1) 
		port = PORT_NUM;
	else 
		return 1;
	printf("    Server v0.01\n");
	lsock = CreateSocket(port);
	if (lsock == -1)
		return 1;
	for(;;) { /**/
		fd_set readfds;
		int maxDs = lsock;
		int res;
		FD_ZERO(&readfds);
		FD_SET(lsock, &readfds);
		struct ClientNode * cl, *prev;
		for (cl = clients; cl!=NULL; cl = cl->next) {
			FD_SET(cl->fd, &readfds);
			if(cl->fd > maxDs)
				maxDs = cl->fd;
		}
		res = select(maxDs+1, &readfds, NULL, NULL, NULL);
		if(res<1) {
			/*handle error*/
		}
		if(FD_ISSET(lsock, &readfds)) {	
			clients = AddClient(lsock, clients);
			printf("new client connected\n");
		}
		prev = clients;
		for(cl = clients; cl != NULL; cl = cl->next) {
			if(FD_ISSET(cl->fd, &readfds)) {
				/*read data, process it*/
				int status;
				status = ProcessClient(cl);
				if(1 == status) { 
					cl = DeleteClient(cl, prev, &clients);
					if(cl == NULL)
						break;
				}
			}
			prev = cl;
		}
	}
	return 0;
}
