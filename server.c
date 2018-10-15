#include <server.h>

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

int createListenFd(void) {
    int listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        perror("socket create failed");
        exit(-1);
    }
    return listenFd;
}

void bindListenFd(int listenFd, struct sockaddr* server) {
    int bindResult = bind(listenFd, server, sizeof(struct sockaddr_in));

    if (bindResult == -1) {
        perror("socket bind failed");
        exit(-1);
    }
    return;
}

void startListenListenFd(int listenFd) {
    int listenResult = listen(listenFd, BACK_LOG);

    if (listenResult == -1) {
        perror("socket listen failed");
        exit(-1);
    }
    return;
}

_Noreturn void serverStart(void) {
    int listenFd;
    struct sockaddr_in server;
    listenFd = createListenFd();
    setNonblocking(listenFd);
    int socketOpt = SO_REUSEADDR;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &socketOpt, sizeof (socketOpt));
    setServerStruct(&server);
    bindListenFd(listenFd, (struct sockaddr*)&server);
    startListenListenFd(listenFd);
    printf("create listenFd successful\n");

    //线程分离的属性
//    pthread_attr_t attr;
//    pthread_attr_init(&attr);
//    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

//    while (true) {
//        int connectFd = accept(listenFd, NULL, NULL);
//        if (connectFd == -1) {
//            perror("socket accept failed");
//            continue;
//        }
//        struct Connect* connect= malloc(sizeof (struct Connect));
//        connect->sockFd = connectFd;
//        pthread_t pthreadId;
//        pthread_create(&pthreadId, &attr, &processCli, connect);
//    }
//    pthread_attr_destroy(&attr);

    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        perror("epoll create failed");
        exit(-1);
    }
    struct epoll_event event;
    struct epoll_event *events;
    event.data.fd = listenFd;
    event.events = EPOLLIN | EPOLLET | EPOLLOUT;
    int epollControlResult = epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &event);
    if (epollControlResult == -1) {
        perror("epoll control add event failed");
        exit(-1);
    }
#define MAXEVENTS 64
    events = malloc(MAXEVENTS * sizeof(struct epoll_event));
    if (events == NULL) {
        perror("epoll malloc events failed");
        exit(-1);
    }

    pthread_attr_t attr;
    printf("epoll create successful\n");
    while (true) {
        int epollEventCounts = epoll_wait (epollFd, events, MAXEVENTS, -1);

        for (int i = 0; i < epollEventCounts; ++i) {
            if (events[i].data.fd == listenFd) {
                pthread_t pthreadId;
                struct Connect* connect= malloc(sizeof (struct Connect));
                connect->sockFd = events[i].data.fd;
                pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
                pthread_create(&pthreadId, &attr, &processNewConnection, connect);
//                pthread_detach(pthreadId);
                pthread_attr_destroy(&attr);
            }
        }
    }
}

void swap(char* lhs, char* rhs) {
    char temp = *lhs;
    *lhs = *rhs;
    *rhs = temp;
}

void exchange(char* srcString, unsigned length) {
    for(unsigned i = 0; i < length/2; ++i) {
        swap(srcString + i, srcString + length - i - 1);
    }
}

_Noreturn void* processNewConnection(void* connectPtr) {
#define BUFFSIZE 2048
    char buffer[BUFFSIZE], clientName[BUFFSIZE];
    char fromAddress[BUFFSIZE];
    struct sockaddr_in client;
    ssize_t reciveLength;
    socklen_t length = sizeof(client);
    bzero(&client, sizeof(struct sockaddr_in));
    struct Connect connect;
    memcpy(&connect, connectPtr, sizeof(struct Connect));
    free(connectPtr);
    int connectFd = accept(connect.sockFd, (struct sockaddr*)(&client), &length);
    if(connectFd == -1) {
        perror("epoll accept successful");
        pthread_exit(NULL);
    }
    memcpy(fromAddress, inet_ntoa(client.sin_addr), strlen(inet_ntoa(client.sin_addr)));

    printf("Got a connection from %s\n", fromAddress);
    reciveLength = recv(connectFd, buffer, BUFFSIZE, 0);
    if (reciveLength == -1) {
        close(connectFd);
        pthread_exit(NULL);
    }
    memcpy(clientName, buffer, (size_t)reciveLength);
    clientName[reciveLength - 1] = '\0';
    printf("%s client's name is %s\n", fromAddress, clientName);

    while (true) {
        reciveLength = recv(connectFd, buffer, BUFFSIZE, 0);
        if (reciveLength == -1)
            continue;
        buffer[reciveLength - 1] = '\0';
#define BYE "bye"
        if (memcmp(buffer, BYE, sizeof(BYE) -1) == 0 && reciveLength == 5) {
            close(connectFd);
            printf("client %s from %s end\n", clientName, inet_ntoa(client.sin_addr));
            pthread_exit(NULL);
        }
        printf("%s: %s\n", clientName, buffer);
//        printf("%s length: %ld\n", buffer, reciveLength);
//        for (unsigned i = 0; i < strlen(buffer) + 1; ++i) {
//            printf("%d ", buffer[i]);
//        }
//        printf("\n");
        exchange(buffer, (unsigned)(length - 2));
        send(connectFd, buffer, (size_t)length, 0);
    }
}
