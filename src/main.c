/*todo:finish commands */


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


void Broadcast(const char * msg, struct Game * game,char finish)
{
	struct ClientNode * list = game->cls;
	while(list) {
		SendMessage(msg, list, finish);
		list = list->next;
	}
}

int ExecuteCommand(const char * cmd, struct ClientNode * client)
{
	int argc,i;

	char ** argv = ParseCmd(cmd, &argc);
	if(argv[0] == NULL) {
		free(argv);
		return 0;
	}
	if(!strcmp(argv[0],"/help"))
		SHelp(client);
	else if(!strcmp(argv[0], "/start"))
		Start(argc, argv, client);
	else if(!strcmp(argv[0], "/info"))
		Info(argc, argv, client);
	else if(!strcmp(argv[0], "/name"))
		Name(argc, argv, client);
	else if(!strcmp(argv[0], "market"))
		Market(argc, argv, client);
	else if(!strcmp(argv[0], "player"))
		Player(argc, argv, client);
	else if(!strcmp(argv[0], "prod"))
		Prod(argc, argv, client);
	else if(!strcmp(argv[0], "turn"))
		Turn(argc, argv, client);
	else if(!strcmp(argv[0], "buy"))
		Buy(argc, argv, client);
	else if(!strcmp(argv[0], "sell"))
		Sell(argc, argv, client);
	else if(!strcmp(argv[0], "build"))
		Build(argc, argv, client);
	else if(!strcmp(argv[0], "help"))
		Help(client);
	else
		SendMessage("Unknown command, use /help", client, 1);
	printf("%s: %s\n", client->name, cmd);
	for(i = 0; argv[i]!=NULL; i++)
		free(argv[i]);
	free(argv);
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
struct ClientNode * AddClient(int lsock, struct ClientNode * list, struct Game * game) 
{
	char name[50];

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
	newcl->game = game;
	(newcl->name)[0] = ' ';
	(newcl->name)[1] = '\0';
	sprintf(name, "/name %s%i", "player", counter);
	counter++;
	ExecuteCommand(name, newcl);
	if(game->state) {
		SendMessage("Sorry, game have already started", newcl, 1);
		shutdown(newcl->fd, 2);
		close(newcl->fd);
		free(newcl->buff);
		free(newcl);
		return list;
	}

	newcl->next = list;
	(game->playersCount)++;
	return newcl;
}


struct ClientNode * DeleteClient(struct ClientNode * cl, 
				struct ClientNode * prev, struct ClientNode ** clients)
{
	printf("client disconnected\n");
	(cl->game->playersCount)--;
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
	struct Game * game;
	struct Game testgame;
	game = &testgame;
	game->playersCount = 0;
	game->state = 0;
	game->cls = NULL;
	int port;
	if(argc == 1) 
		port = PORT_NUM;
	else
		port = atoi(argv[1]);
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
		for (cl = game->cls; cl!=NULL; cl = cl->next) {
			FD_SET(cl->fd, &readfds);
			if(cl->fd > maxDs)
				maxDs = cl->fd;
		}
		res = select(maxDs+1, &readfds, NULL, NULL, NULL);
		if(res<1) {
			/*handle error*/
			perror("select");
			return 1;
		}
		if(FD_ISSET(lsock, &readfds)) {	
			game->cls = AddClient(lsock, game->cls, game);
			printf("new client connected\n");
		}
		prev = game->cls;
		for(cl = game->cls; cl != NULL; cl = cl->next) {
			if(FD_ISSET(cl->fd, &readfds)) {
				/*read data, process it*/
				int status;
				status = ProcessClient(cl);
				if(1 == status) { 
					cl = DeleteClient(cl, prev, &game->cls);
					if(cl == NULL)
						break;
				}
			}
			prev = cl;
		}
	}
	return 0;
}
