#define _GNU_SOURCE
#include <pthread.h>        /* for POSIX threads */
#include <sys/socket.h> /* for socket(), bind(), and connect() | recv() and send()*/
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "NoticePeriod.h"
#include "NoticeDay.h"
#include "ListBreakThread.h"

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define MAXRECVSIZE 58 /* 8 bit for time, 50 bit for text*/
#define SEND_NOTICE_SIZE 50 /* количество символов в напоминании максимум*/
#define PORT 5250  /* Порт который слушаем и отправялем на который напоминания*/

struct Notice {
    int time_begin;
    int time_period;
    char text[SEND_NOTICE_SIZE];
};

/* Structure of arguments to pass to client thread */
struct ThreadArgs {
    int clntSock; /* Socket descriptor for client */
    struct sockaddr_in clntAddr; /* ip Address Client */
    struct ListBreakThread *listBreak; // id of list connect client
};

/* Поток для создания потоков при подключении клиентов по TCP */
void *ThreadTCP(void *argsTcp);
/* Поток при подключении клиетов по TCP */
void *tellClientTcp(void *arg);
/* Отправка уведомления клинету по UDP */
void sendNoticeClient(struct ThreadArgs *argsClient, struct QueueNoticeDay *id_queue_day, struct QueueNoticePeriod *id_queue_period, int id);
/* Получение напоминания(текста) если пришло время */
char *getNotice(struct QueueNoticeDay *, struct QueueNoticePeriod *);
/* Создание TCP сокета */
int CreateServerTCPSocket();
/* Создание UDP сокета */
int CreateServerUDPSocket(int id_client, struct sockaddr_in * clntAddr);
/* Обеспечение связи сервера и клиента по TCP */
int HandleTCPClient(int servSock, struct sockaddr_in * clntAddr);
/* Проверка соединения */
int checkConnection(int clntSocket, struct sockaddr_in clntAddr);
/* Критические ошибки и аварийное завершение */
void DieWithError(char *errorMessage);

int main(int argc, char *argv[]) {
    int *servSocketTcp; /* Socket descriptor for server */
    pthread_t threadID;

    servSocketTcp = (int *) malloc(sizeof (int));
    *servSocketTcp = CreateServerTCPSocket();
    if (pthread_create(&threadID, NULL, ThreadTCP, (void *) servSocketTcp) != 0)
        DieWithError("pthread_create() failed");
    pthread_join(threadID, NULL);

    return 0;
}

int CreateServerTCPSocket() {
    int sock; /* socket to create */
    struct sockaddr_in servAddr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof (servAddr)); /* Zero out structure */
    servAddr.sin_family = AF_INET; /* Internet address family */
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    servAddr.sin_port = htons(PORT); /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &servAddr, sizeof (servAddr)) < 0)
        DieWithError("bind() failed");

    return sock;
}

int HandleTCPClient(int servSock, struct sockaddr_in * clntAddr) {
    int clntSock; /* Socket descriptor for client */
    struct sockaddr_in ClntAddr; /* Client address */
    unsigned int clntLen; /* Length of client address data structure */

    /* Set the size of the in-out parameter */
    clntLen = sizeof (ClntAddr);

    /* Wait for a client to connect */
    if ((clntSock = accept(servSock, (struct sockaddr *) &ClntAddr, &clntLen)) < 0)
        DieWithError("accept() failed");
    *clntAddr = ClntAddr;
    return clntSock;
}

int CreateServerUDPSocket(int id_client, struct sockaddr_in * clntAddr) {
    int sock; /* socket to create */
    struct sockaddr_in ClntAddr; /* Local address */
    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&ClntAddr, 0, sizeof (ClntAddr)); /* Zero out structure */
    ClntAddr.sin_family = AF_INET; /* Internet address family */
    ClntAddr.sin_port = htons(PORT); /* Local port */
    ClntAddr.sin_addr = clntAddr->sin_addr;
    *clntAddr = ClntAddr;

    return sock;
}

void *ThreadTCP(void *argsTcp) {
    int clntSock;
    int servSock;
    struct sockaddr_in clntAddr;
    struct ThreadArgs *argsClient;
    struct ListBreakThread *listBreak;
    pthread_t threadID;

    servSock = *(int *) argsTcp;
    listBreak = createListBreak();
    memset(listBreak, 0, sizeof (struct ListBreakThread));

    pthread_detach(pthread_self());
    listen(servSock, MAXPENDING);
    while (1) {
        clntSock = HandleTCPClient(servSock, &clntAddr);
        if (clntSock >= 0) {
            argsClient = (struct ThreadArgs*) malloc(sizeof (struct ThreadArgs));
            argsClient -> clntSock = clntSock;
            argsClient -> listBreak = listBreak;
            argsClient -> clntAddr = clntAddr;
            // Create Thread for recive message from client
            pthread_create(&threadID, NULL, tellClientTcp, (void *) argsClient);
        }
    }
    freeListBreak(listBreak);
    return (NULL);
}

void * tellClientTcp(void *argsClient) {
    int clntSocket; /* Socket descriptor for client connection */
    struct ListBreakThread *listBreak;
    struct Notice *note;
    int id; // unique id client - number client connection with server
    struct QueueNoticePeriod *id_queue_period = NULL;
    struct QueueNoticeDay *id_queue_day = NULL;

    pthread_detach(pthread_self());
    note = (struct Notice *) malloc(sizeof (struct Notice));
    memset(note, 0, sizeof (struct Notice));
    listBreak = ((struct ThreadArgs *) argsClient) -> listBreak;
    clntSocket = ((struct ThreadArgs *) argsClient) -> clntSock;

    /* get message from client, need id for client*/
    recv(clntSocket, note, MAXRECVSIZE, 0);
    switch (note->time_begin) {
        case -1: // if, that recv notice from client
            id = addConnectClient(listBreak);
            note->time_begin = id;
            send(clntSocket, note, MAXRECVSIZE, 0); //send id to client from server
            while (1) {
                recv(clntSocket, note, MAXRECVSIZE, 0); /* Recive notice form client */
                note->text[strlen(note->text)] = '\0';
                /* прием уведомлений завершен и выход из цикла */
                if (note->time_begin < 0)
                    break;
                /* create queue notice(day, period) */
                if (note->time_period == 0) { /* day notice */
                    if (id_queue_day == NULL)
                        id_queue_day = createQueueNoticeDay();
                    addNoticeDay(id_queue_day, note->text, note->time_begin);
                } else { /* period notice */
                    if (id_queue_period == NULL)
                        id_queue_period = createQueueNoticePeriod();
                    addNoticePeriod(id_queue_period, note->text, note->time_begin, note->time_period);
                }
            }
            close(clntSocket); // close connection client with 'tcp'
            sendNoticeClient(argsClient, id_queue_day, id_queue_period, id);
            break;
        case -2: // if, that Delete queue's notice
            id = note->time_period;
            close(clntSocket);
            clientBreak(listBreak, id, 1);
            break;
    }
    free(argsClient);
    return (NULL);
}

void sendNoticeClient(struct ThreadArgs *argsClient, struct QueueNoticeDay *id_queue_day,
        struct QueueNoticePeriod *id_queue_period, int id) {
    int clntSocket; /* Socket descriptor for client connection */
    struct ListBreakThread *listBreak;
    struct sockaddr_in clntAddr;
    char *sendString;
    int sendStringLen;
    int err = 0;
    int exit;
    
    listBreak = ((struct ThreadArgs *) argsClient) -> listBreak;
    clntAddr = ((struct ThreadArgs *) argsClient) -> clntAddr;
    clntSocket = CreateServerUDPSocket(id, &clntAddr);
    while (1) {
        if ((err = checkConnection(clntSocket, clntAddr)) != 0) {// mistakes
            clientBreak(listBreak, id, 1);
            printf("client with id <%d> connection failed\n", id);
            break;
        }
        
        sendString = getNotice(id_queue_day, id_queue_period);
        if (sendString != NULL) {
            sendStringLen = strlen(sendString);
            if (sendto(clntSocket, sendString, sendStringLen, 0, (struct sockaddr *)
                    &clntAddr, sizeof (clntAddr)) != sendStringLen) {
            }
            free(sendString);
        }
        exit = clientBreak(listBreak, id, 0);
        if (exit != 0) {
            /* send empty string with 1 symbols backspace, 
             * that client break from recvfrom */
            char sendStr[2];
            sendStr[0] = ' ';
            sendStr[1] = '\0';
            sendStringLen = strlen(sendStr);
            if (sendto(clntSocket, sendStr, sendStringLen, 0, (struct sockaddr *)
                    &clntAddr, sizeof (clntAddr)) != sendStringLen)
            break;
        }
        sleep(1);
    }
    freeQueueNoticeDay(id_queue_day);
    freeQueueNoticePeriod(id_queue_period);
    freeConnectClient(listBreak, id);
    close(clntSocket);
}

int checkConnection(int clntSocket, struct sockaddr_in clntAddr) {
    int fail_timeout = 120;
    int err;
    char checkConnectStr[2];
    checkConnectStr[0] = '.';
    checkConnectStr[1] = '\0';
    int checkConnectlen = strlen(checkConnectStr);
        
    while (1) {
        err = sendto(clntSocket, checkConnectStr, checkConnectlen, 0, (struct sockaddr *)
                &clntAddr, sizeof (clntAddr));
        if (err != checkConnectlen) {
            printf("fail\n");
            fail_timeout--;
            sleep(1);
        } else {
            err = 0;
            break;
        }
        
        if (fail_timeout < 0) {
            err = 1;
            break;
        }
    }
    return err;
}

char * getNotice(struct QueueNoticeDay *id_queue_day,
        struct QueueNoticePeriod * id_queue_period) {
    int time_sec; // time in seconds in the moment
    char *str_day, *str_period, *str;
    int str_day_len = 0, str_period_len = 0, slen = 0;
    time_t time_now;
    struct tm *timest;
    time_now = time(NULL);
    timest = localtime(&time_now);
    time_sec = timest->tm_hour * 60 * 60 + timest->tm_min * 60 + timest->tm_sec;

    str_day = returnNoticeDay(id_queue_day, time_sec);
    str_period = returnNoticePeriod(id_queue_period, time_sec);
    if (str_day != NULL) {
        str_day_len = strlen(str_day);
    }
    if (str_period != NULL) {
        str_period_len = strlen(str_period);
    }

    if (str_day_len > 0 && str_period > 0) { // Exist two reminds: period, day
        slen = str_day_len + str_period_len;
        str = (char *) malloc(slen + 1);
        strcat(str, str_day);
        strcat(str, str_period);
        free(str_day);
        free(str_period);
    } else if (str_day_len > 0) { // Exist day reminds
        str = str_day;
        free(str_period);
    } else if (str_period_len > 0) { // Exist period reminds
        str = str_period;
        free(str_day);
    } else { // Exist 0 reminds  
        free(str_day);
        free(str_period);
        str = NULL;
    }
    return str;
}

void DieWithError(char *errorMessage) {
    perror(errorMessage);
    exit(1);
}