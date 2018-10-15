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
//    setNonblocking(listenFd);

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

    //线程分离的属性
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    while (true) {
        int connectFd = accept(listenFd, NULL, NULL);
        if (connectFd == -1) {
            perror("socket accept failed");
            continue;
        }
        struct Connect* connect= malloc(sizeof (struct Connect));
        connect->sockFd = connectFd;
        pthread_t pthreadId;
        pthread_create(&pthreadId, &attr, &processCli, connect);
    }
    pthread_attr_destroy(&attr);

//    int epollFd = epoll_create1(0);
//    if (epollFd == -1) {
//        perror("epoll create failed");
//        exit(-1);
//    }
//    struct epoll_event event;
//    struct epoll_event *events;
//    event.data.fd = listenFd;
//    event.events = EPOLLIN | EPOLLET | EPOLLOUT;
//    int epollControlResult = epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &event);
//    if (epollControlResult == -1) {
//        perror("epoll control add event failed");
//        exit(-1);
//    }
//#define MAXEVENTS 64
//    events = malloc(MAXEVENTS * sizeof (struct epoll_event));
//    if (events == NULL) {
//        perror("epoll malloc events failed");
//        exit(-1);
//    }

//    printf("epoll create successful");
//    while (true) {
//        int epollEventCounts = epoll_wait (epollFd, events, MAXEVENTS, -1);

//        for (int i = 0; i < epollEventCounts; ++i) {
//            pthread_t pthreadId;
//            struct Connect* connect= malloc(sizeof (struct Connect));
//            connect->sockFd = events[i].data.fd;
//            pthread_create(&pthreadId, NULL, &processCli, connect);
//            pthread_detach(pthreadId);
//        }
//    }
}

_Noreturn void* processCli(void* threadArgs) {
    struct sockaddr_in client;
    bzero(&client, sizeof(struct sockaddr_in));
    struct Connect* connectPtr = (struct Connect*)threadArgs;
    struct Connect connect;
    memcpy(&connect, connectPtr, sizeof(struct Connect));
    free(connectPtr);
    int connectFd = connect.sockFd;
#define BUFFSIZE 2048
    char buffer[BUFFSIZE];

    while (true) {
        bzero(buffer, sizeof(buffer));
        ssize_t length = recv(connectFd, buffer, BUFFSIZE - 1, 0);
        if (length == -1)
            continue;
#define BYE "bye"
        if (memcmp(buffer, BYE, sizeof(BYE) -1) == 0) {
            close(connectFd);
            pthread_exit(NULL);
        }
        printf("%s length: %d\n", buffer, (int)strlen(buffer));
        for (unsigned i = 0; i < strlen(buffer) + 1; ++i) {
            printf("%d ", buffer[i]);
        }
        printf("\n");
        send(connectFd, buffer, (size_t)length, 0);
    }
}
