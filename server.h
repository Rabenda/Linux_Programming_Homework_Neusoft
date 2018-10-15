#ifndef SERVER_H
#define SERVER_H
#include <config.h>

#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>

#include <sys/epoll.h>
#include <threads.h>

void setServerStruct(struct sockaddr_in* server);
_Noreturn void serverStart(void);
void* processCli(void* threadArgs);
#endif // SERVER_H
