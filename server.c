#include <server.h>

void setServerStruct(struct sockaddr_in* server) {
    server->sin_family = AF_INET;
    server->sin_port = htons(SERVER_PORT);
    server->sin_addr.s_addr= htonl(INADDR_ANY);
}

_Noreturn void serverStart(void) {
    int listenFd, connectionFd;

    struct sockaddr_in server, client;
    uint clientLen = sizeof (client);
    int socketOpt = SO_REUSEADDR;

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        perror("socket create failed");
        exit(-1);
    }

    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof (socketOpt));
    bzero(&server, sizeof (server));
    setServerStruct(&server);
    int bindResult = bind(listenFd, (struct sockaddr*)&server, sizeof (server));

    if (bindResult == -1) {
        perror("socket bind failed");
        exit(-1);
    }

    int listenResult = listen(listenFd, BACK_LOG);

    if (listenResult == -1) {
        perror("socket listen failed");
        exit(-1);
    }

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll create failed");
        exit(-1);
    }
    struct epoll_event event;
    struct epoll_event *events;
    event.data.fd = listenFd;
    event.events = EPOLLIN | EPOLLET;
    int epollControlResult = epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &event);
    if (epollControlResult == -1) {
        perror("epoll control add event failed");
        exit(-1);
    }
#define MAXEVENTS 64
    events = malloc(MAXEVENTS * sizeof (struct epoll_event));
    if (events == NULL) {
        perror("epoll malloc events failed");
        exit(-1);
    }



    while (true) {
        int epollEventCounts = epoll_wait (epollFd, events, MAXEVENTS, -1);

//        connectionFd = accept(listenFd, (struct sockaddr*)& client, &clientLen);
//        if (connectionFd == -1) {
//            perror("socket accept failed");
//            exit(-1);
//        }

        pthread_t pthreadId;

        pthread_create(&pthreadId, NULL, &processCli, NULL);


    }
}

void* processCli(void* threadArgs) {
    (void)(threadArgs);
    return NULL;
}
