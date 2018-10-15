#include <server.h>

struct Connect {
    int sockFd;
};

void setServerStruct(struct sockaddr_in* server) {
    bzero(server, sizeof(struct sockaddr_in));
    server->sin_family = AF_INET;
    server->sin_port = htons(SERVER_PORT);
    server->sin_addr.s_addr= htonl(INADDR_ANY);
}

void setNonblocking(int sockFd)
{
    int opts;
    if ((opts = fcntl(sockFd, F_GETFL)) < 0)
        exit(-1);
    opts = opts | O_NONBLOCK;
    if (fcntl(sockFd, F_SETFL, opts) < 0)
        exit(-1);
}

_Noreturn void serverStart(void) {
    int listenFd;

    struct sockaddr_in server;
    int socketOpt = SO_REUSEADDR;

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        perror("socket create failed");
        exit(-1);
    }
    setNonblocking(listenFd);

    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof (socketOpt));
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

    printf("create listenFd successful");
#define MAXEVENTS 64
    int epollFd = epoll_create(MAXEVENTS);
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

    events = malloc(MAXEVENTS * sizeof (struct epoll_event));
    if (events == NULL) {
        perror("epoll malloc events failed");
        exit(-1);
    }

    printf("epoll create successful");

    while (true) {
        int epollEventCounts = epoll_wait (epollFd, events, MAXEVENTS, -1);

        for (int i = 0; i < epollEventCounts; ++i) {
            pthread_t pthreadId;
            struct Connect* connect= malloc(sizeof (struct Connect));
            connect->sockFd = events[i].data.fd;
            pthread_create(&pthreadId, NULL, &processCli, connect);
            pthread_detach(pthreadId);
        }
    }
}

_Noreturn void* processCli(void* threadArgs) {
    struct sockaddr_in client;
    bzero(&client, sizeof(struct sockaddr_in));
    struct Connect* connectPtr = (struct Connect*)threadArgs;
    struct Connect connect;
    memcpy(&connect, connectPtr, sizeof(struct Connect));
    free(connectPtr);
    int connectFd = accept(connect.sockFd, (struct sockaddr*)&client, sizeof(client));
    if (connectFd == -1) {
        perror("epoll accept events failed");
        pthread_exit(NULL);
    }
#define BUFFSIZE 2048
    char buffer[BUFFSIZE];

    while (true) {
        ssize_t length = recv(connectFd, buffer, BUFFSIZE - 1, 0);
        if (length == -1)
            continue;
        buffer[length] = '\0';
        if (strcasecmp(buffer, "bye") == 0) {
            close(connectFd);
            pthread_exit(NULL);
        }
        send(connectFd, buffer, (size_t)length, 0);
    }
}
