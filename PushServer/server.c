#define _GNU_SOURCE
#include <pthread.h>        /* for POSIX threads */
#include <sys/socket.h> /* for socket(), bind(), and connect() | recv() and send()*/
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <string.h>     /* for memset() */
#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */
#include <unistd.h>     /* for close() */
#include <time.h>
#include <arpa/inet.h>

#include "NoticePeriod.h"
#include "NoticeDay.h"
#include "ListBreakThread.h"

#define MAXPENDING 5    /* Maximum outstanding connection requests */
#define MAXRECVSIZE 58
#define SEND_NOTICE_SIZE 50
#define SERVER "127.0.0.1"
#define PORT 5250  

void *ThreadTCP(void *argsTcp);
void *tellClientTcp(void *arg);
char *getSendNotice(struct QueueNoticeDay *, struct QueueNoticePeriod *);

int CreateServerTCPSocket();
int CreateServerUDPSocket(int id_client);
int HandleTCPClient(int servSock, struct sockaddr_in * clntAddr);
void DieWithError(char *errorMessage);

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

int main(int argc, char *argv[]) {
    int servSocketTcp; /* Socket descriptor for server */
    pthread_t threadID;

    servSocketTcp = CreateServerTCPSocket();

    if (pthread_create(&threadID, NULL, ThreadTCP, (void *) servSocketTcp) != 0)
        DieWithError("pthread_create() failed");
    pthread_join(threadID, NULL);

    return 0;
}

int CreateServerTCPSocket() {
    int sock; /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */

    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&ServAddr, 0, sizeof (ServAddr)); /* Zero out structure */
    ServAddr.sin_family = AF_INET; /* Internet address family */
    ServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    ServAddr.sin_port = htons(PORT); /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &ServAddr, sizeof (ServAddr)) < 0)
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

int CreateServerUDPSocket(int id_client) {
    int sock; /* socket to create */
    struct sockaddr_in ClntAddr; /* Local address */
    /* Create socket for incoming connections */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&ClntAddr, 0, sizeof (ClntAddr)); /* Zero out structure */
    ClntAddr.sin_family = AF_INET; /* Internet address family */
    ClntAddr.sin_port = htons(PORT + id_client - 1); /* Local port */
    if (inet_aton(SERVER, &ClntAddr.sin_addr) == 0)
        DieWithError("inet_aton() failed");

    return sock;
}

void *ThreadTCP(void *argsTcp) {
    int clntSock;
    int servSock;
    struct sockaddr_in clntAddr;
    struct ThreadArgs *argsClient;
    struct ListBreakThread *listBreak;
    pthread_t threadID;

    servSock = (int) argsTcp;
    listBreak = (struct ListBreakThread *) malloc(sizeof (struct ListBreakThread));
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
            pthread_create(&threadID, NULL, tellClientTcp, (void *) argsClient);
        }
    }
    return (NULL);
}

void * tellClientTcp(void *argsClient) {
    int clntSocket; /* Socket descriptor for client connection */
    struct ListBreakThread *listBreak;
    struct sockaddr_in clntAddr;
    struct Notice *note;
    int id; // unique id client - number client connection with server
    char *sendString;
    int sendStringLen;
    struct QueueNoticePeriod *id_queue_period;
    struct QueueNoticeDay *id_queue_day;

    pthread_detach(pthread_self());
    note = (struct Notice *) malloc(sizeof (struct Notice));
    memset(note, 0, sizeof (struct Notice));
    clntSocket = ((struct ThreadArgs *) argsClient) -> clntSock;
    clntAddr = ((struct ThreadArgs *) argsClient) -> clntAddr;
    listBreak = ((struct ThreadArgs *) argsClient) -> listBreak;
    free(argsClient);

    /* get message from client, need id for client*/
    recv(clntSocket, note, MAXRECVSIZE, 0); /* Recive notice from client */
    if (note->time_begin == -1) { // if, that Delete queue's notice
        id = note->time_period;
        close(clntSocket);
        clientBreak(listBreak, id, 1);
    } else if (note->time_begin == -2) { // if, that recv notice from client
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

        /* Send notice to client with 'udp' */
        clntSocket = CreateServerUDPSocket(id);
        while (1) {
            sendString = getSendNotice(id_queue_day, id_queue_period);
            if (sendString != NULL) {
                sendStringLen = strlen(sendString);
                if (sendto(clntSocket, sendString, sendStringLen, 0, (struct sockaddr *)
                        &clntAddr, sizeof (clntAddr)) != sendStringLen)
                    DieWithError("sendto() sent a different number of bytes than expected");
                free(sendString);
            }
            int exit = clientBreak(listBreak, id, 0);
            if (exit != 0) {
                /* send empty string with 1 symbols backspace, 
                 * that client break from recvfrom */
                char sendStr[2];
                sendStr[0] = ' ';
                sendStr[1] = '\0';
                sendStringLen = strlen(sendStr);
                if (sendto(clntSocket, sendStr, sendStringLen, 0, (struct sockaddr *)
                        &clntAddr, sizeof (clntAddr)) != sendStringLen)
                    DieWithError("sendto() sent a different number of bytes than expected");
                break;
            }
            sleep(60);
        }
        // delete queue notice for this client
        freeQueueNoticeDay(id_queue_day);
        freeQueueNoticePeriod(id_queue_period);
        freeConnectClient(listBreak, id);
        close(clntSocket);

    }
    return (NULL);
}

char * getSendNotice(struct QueueNoticeDay *id_queue_day,
        struct QueueNoticePeriod *id_queue_period) {
    int time_sec;
    char *str_day, *str_period, *str;
    int str_day_len = 0, str_period_len = 0, slen = 0;
    time_t time_now;
    struct tm *timest;
    time_now = time(NULL);
    timest = localtime(&time_now);
    time_sec = timest->tm_hour * 60 * 60 + timest->tm_min * 60;

    str_day = returnNoticeDay(id_queue_day, time_sec);
    str_period = returnNoticePeriod(id_queue_period, time_sec);
    if (str_day != NULL) {
        str_day_len = strlen(str_day);
    }
    if (str_period != NULL) {
        str_period_len = strlen(str_period);
    }
    if (str_day_len > 0 && str_period > 0) {
        slen = str_day_len + str_period_len;
        str = (char *) malloc(slen + 1);
        strcat(str, str_day);
        strcat(str, str_period);
        free(str_day);
        free(str_period);
    } else if (str_day > 0) {
        str = str_day;
        free(str_period);
    } else if (str_period > 0) {
        str = str_period;
        free(str_day);
    } else { //length strings : str_day and str_period equals 0  
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
