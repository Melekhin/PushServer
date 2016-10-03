#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <arpa/inet.h>

#define SEND_NOTICE 58  /* 8 bit for time, 50 bit for text */
#define RECV_NOTICE_SIZE 50
#define PORT 5250
#define COUNTPORT 10

struct Notice {
    int time_begin;
    int time_period;
    //char *text;
    char text[RECV_NOTICE_SIZE];
};

struct ServerSocket {
    struct sockaddr_in serverAddr;
    int port;
    int socket;
    int id;
};
char *server;

int CreateClientUDPSocket(int number_client);
int CreateClientTCPSocket();
int SendNotice(int sock);
void RecvNotice(int id);
void FreeNotice(int id);
int exitNotice(int exit);
void DieWithError(const char *msg);

void sigint(int signo) {
    if (signo == SIGINT) {
        exitNotice(1);
    }
}

int main(int argc, char *argv[]) {
    int id; // unique id client - number client on server
    int sock;
    
    if (argc == 2)
        server = argv[1];
    
    /* exit program on signal SIGKILL, his number 9 */
    struct sigaction s;
    s.sa_handler = sigint;
    s.sa_flags = 0;
    sigemptyset(&s.sa_mask);
    sigaction(SIGINT, &s, NULL);
    
    sock = CreateClientTCPSocket();

    id = SendNotice(sock);
    RecvNotice(id);
    return 0;
}

int SendNotice(int sock) {
    int id;
    int action = 0;
    int hour, min, period;
    struct Notice *note;
    note = (struct Notice *) malloc(sizeof (struct Notice));
    memset(note, 0, sizeof (struct Notice));
    /* get id from server */
    note->time_begin = -2;
    send(sock, note, SEND_NOTICE, 0);
    memset(note, 0, sizeof (struct Notice));
    recv(sock, note, SEND_NOTICE, 0);
    id = note->time_begin;
    printf("id %d\n", id);
    while (action != 3) {
        printf("Select the action:\n");
        printf("\t 1 - add Notice every day\n");
        printf("\t 2 - add Notice with periodic\n");
        printf("\t 3 - complete create notice\n");
        scanf("%d", &action);
        switch (action) {
            case 1:
                printf("Write time notice: <hour> <minutes>\n");
                printf("Example, where 10 -hour, 25 -minutes: 10 25\n");
                printf("Write time notice: ");
                scanf("%d%d", &hour, &min);
                period = 0;
                printf("Write text of notice: ");
                scanf("%s", note->text);
                break;
            case 2:
                printf("Write time notice: <hour> <minutes> <period_minutes>\n");
                printf("Example, where 10 -hour, 25 -minutes, 30 - period: 10 25 30\n");
                printf("Write time notice: ");
                scanf("%d%d%d", &hour, &min, &period);
                printf("Write text of notice: ");
                fgets(note->text, RECV_NOTICE_SIZE, stdin);
                break;
            case 3:
                hour = 0;
                min = -1;
                break;
        }
        note->time_begin = hour * 60 * 60 + min * 60;
        note->time_period = period * 60;
        send(sock, note, SEND_NOTICE, 0);
        scanf("%*[^\n]");
    }
    printf("Notices creates.\n");
    close(sock);
    return id;
}

void RecvNotice(int id) {
    int sock;
    int recvStringlen;
    char *recvString;
    struct sockaddr_in udpAddrRecv, cmpAddr;
    int udpAddrRecv_len;

    recvString = (char*)malloc(RECV_NOTICE_SIZE+1);
    memset(recvString, 0, sizeof(recvString));
    memset(&cmpAddr, 0, sizeof(cmpAddr));
    sock = CreateClientUDPSocket(id);
        /* Receive datagram from neibors host */
    while (1) {
        memset(&udpAddrRecv, 0, sizeof (udpAddrRecv));
        if ((recvStringlen = recvfrom(sock, recvString, RECV_NOTICE_SIZE, 0,
                (struct sockaddr *) &udpAddrRecv, &udpAddrRecv_len)) < 0)
            DieWithError("recvfrom(), failed \n");

        if (strcmp(recvString, " ") != 0) { // exit from while
            // print notice in window use gtk
            GtkWidget *label;
            GtkWidget *window;
            gtk_init(0, 0);
            window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
            gtk_window_set_title(GTK_WINDOW(window), "Hello");
            label = gtk_label_new(recvString);
            gtk_container_add(GTK_CONTAINER(window), label);
            gtk_widget_show_all(window);
            g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
            gtk_main();
        }
        if (exitNotice(0) == 0){  /* signal KILL -> exit from while*/
            FreeNotice(id);
            break;   
        }
    }
    free(recvString);
    return;
}

void FreeNotice(int id) {
    int sock;
    struct Notice note;
    memset(&note, 0, sizeof (note));
    sock = CreateClientTCPSocket();

    note.time_begin = -1;
    note.time_period = id;
    send(sock, &note, sizeof (note), 0);
    close(sock);
    return;
}

int CreateClientUDPSocket(int number_client) {
    int sock; /* socket to create */
    struct sockaddr_in serverAddr; /* Local address */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct bind structure */
    memset(&serverAddr, 0, sizeof (serverAddr)); /* Zero out structure */
    serverAddr.sin_family = AF_INET; /* Internet address family */
    if (inet_aton(server, &serverAddr.sin_addr) == 0)
        DieWithError("inet_aton() failed");
    
    /* Bind to the broadcast port */
    serverAddr.sin_port = htons(PORT + number_client -1); /* Server port */
    if (0 > bind(sock, (struct sockaddr *) &serverAddr, sizeof (serverAddr)))
        DieWithError("bind failed(). Max client.");
    
    return sock;
}

int CreateClientTCPSocket() {
    int sock; /* socket to create */
    struct sockaddr_in ServAddr; /* Local address */

    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    /* Construct the server address structure */
    memset(&ServAddr, 0, sizeof (ServAddr)); /* Zero out structure */
    ServAddr.sin_family = AF_INET; /* Internet address family */
    if (inet_aton(server, &ServAddr.sin_addr) == 0)
        DieWithError("inet_aton() failed");
    ServAddr.sin_port = htons(PORT); /* Server port */

    /* Establish the connection to the server */
    if (connect(sock, (struct sockaddr *) &ServAddr, sizeof (ServAddr)) < 0)
        DieWithError("connect() failed");

    return sock;
}

int exitNotice(int exit) {
    static int stop = 1;
    if (exit == 1)
        stop--;
    return stop;
}

void DieWithError(const char *msg) {
    perror(msg);
    exit(0);
}