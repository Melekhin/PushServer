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
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>

#define SEND_NOTICE 58  /* 8 bit for time, 50 bit for text */
#define RECV_NOTICE_SIZE 50 /* максимальное количество символов в напоминании*/ 
#define PORT 5250 /* Порт который слушаем */

struct Notice {
    int time_begin;
    int time_period;
    char text[RECV_NOTICE_SIZE];
};

char *server; // Ip адрес сервера

int CreateClientUDPSocket(); // Создание UDP сокета
int CreateClientTCPSocket(); // Создание TCP сокета
int SendNotice(int sock); // Отправка сообщений на серврер
int fillNotice(); //Заполнение напоминания данными, перед отправкой на сервер
void RecvNotice(); // Ожидаем напоминаний по времени
void FreeNotice(int id); // Удаление напоминаний на сервере
int getDisplayMode(); // Режим отображения напоминаний
void printNoticeGUI(char * str); // отображаем напоминания в ГУИ
void printNoticeCLI(char * str); // отображаем напоминания в консоли
int exitNotice(int exit); // завершение напоминаний, сообщаем об этом серверу
int getId(int id); // получить идеинтификатор клиента
void DieWithError(const char *msg); // Критические ошибки и аварийное завершение

void signint(int signo) { // Обработка сигнала Ctrl^C
    int id;
    exitNotice(1);
    id = getId(0);
    FreeNotice(id);
}

int main(int argc, char *argv[]) {
    int id; // unique id client - number client on server
    int sock;

    if (argc == 2)
        server = argv[1];
    else {
        DieWithError("usage: ./client ip_server\n");
    }
    /* exit program on signal SIGINT */
    struct sigaction s;
    s.sa_handler = signint;
    s.sa_flags = 0;
    sigemptyset(&s.sa_mask);
    sigaction(SIGINT, &s, NULL);

    sock = CreateClientTCPSocket();
    id = SendNotice(sock);
    getId(id); // save unique id in this function in 'flag'
    RecvNotice(id);

    return 0;
}

int SendNotice(int sock) {
    int id;
    int err;
    int action = 0;
    struct Notice *note;
    note = (struct Notice *) malloc(sizeof (struct Notice));
    memset(note, 0, sizeof (struct Notice));
    /* get id from server */
    note->time_begin = -1;
    if ((err = send(sock, note, SEND_NOTICE, 0)) < 0)
        DieWithError("send(), failed.");
    memset(note, 0, sizeof (struct Notice));
    if ((err = recv(sock, note, SEND_NOTICE, 0)) < 0)
        DieWithError("recv(), failed.");
    id = note->time_begin;
    printf("id %d\n", id);
    while (action != 3) {
        memset(note, 0, sizeof (struct Notice));
        action = fillNotice(note);
        if ((err = send(sock, note, SEND_NOTICE, 0)) < 0)
            DieWithError("send(), failed.");
    }
    printf("Notices creates.\n");
    close(sock);
    return id;
}

int fillNotice(struct Notice *note) {
    int action = 0;
    static int count = 0; // Count create Remind's
    int n;
    int hour, min, period;
    while (1) {
        printf("Select the action:\n");
        printf("\t 1 - add Notice every day\n");
        printf("\t 2 - add Notice with periodic\n");
        printf("\t 3 - complete create notice\n");
        n = scanf("%d", &action);
        if (action == 1 || action == 2)
            count++; // exist > than 1 element
        if (n == 1 && action > 0 && action < 4 && count > 0)
            break;
        scanf("%*[^\n]"); // сброс буфера scanf
    }
    switch (action) {
        case 1:
            while (1) {
                printf("Write time notice: <hour(0-23)> <minutes(0-60)>\n");
                printf("Example (wrote two numbers): 10 25\n");
                printf("Write time notice: ");
                n = scanf("%d%d", &hour, &min);
                if ((n == 2) && (hour >= 0 && hour < 24)
                        && (min >= 0 && min <= 60))
                    break;
                scanf("%*[^\n]"); // сброс буфера scanf
            }
            printf("Write text of notice(no more than %d symbols): ",
                    (int) RECV_NOTICE_SIZE);
            while (1) {
                fgets(note->text, RECV_NOTICE_SIZE, stdin);
                if (strcmp(note->text, "\n") != 0) {
                    note->text[strlen(note->text) - 1] = '\0';
                    break;
                }
            }
            period = 0;
            break;
        case 2:
            while (1) {
                printf("Write time notice: <hour(0-23)> <minutes(0-60)> "
                        "<period_minutes(0-1440)>\n");
                printf("Example (wrote three numbers): 10 25 30\n");
                printf("Write time notice: ");
                n = scanf("%d%d%d", &hour, &min, &period);
                if ((n == 3) && (hour >= 0 && hour < 24)
                        && (min >= 0 && min <= 60)
                        && (period > 0 && period < 1440))
                    break;
                scanf("%*[^\n]"); // сброс буфера scanf
            }
            printf("Write text of notice(no more than %d symbols): ",
                    (int) RECV_NOTICE_SIZE);
            while (1) {
                fgets(note->text, RECV_NOTICE_SIZE, stdin);
                if (strcmp(note->text, "\n") != 0) {
                    note->text[strlen(note->text) - 1] = '\0';
                    break;
                }
            }
            break;
        case 3:
            hour = 0;
            min = -1;
            break;
    }
    /* translate time in seconds */
    note->time_begin = hour * 60 * 60 + min * 60;
    note->time_period = period * 60;
    return action;
}

void RecvNotice() {
    int sock = 0;
    int mode = 0;
    int fail_time = 120; // задержка по времени 120 секунд, отклика от сервера
    int recvStringlen = 0;
    char recvString[RECV_NOTICE_SIZE] = {0};
    struct sockaddr_in udpAddrRecv = {0};
    socklen_t udpAddrRecv_len = 0;

    sock = CreateClientUDPSocket();
    fcntl(sock, F_SETFL, O_NONBLOCK);
    mode = getDisplayMode();
    /* Receive datagram from neibors host */
    while (1) {
        memset(recvString, '\0', RECV_NOTICE_SIZE);
        fd_set set;
        struct timeval tv;
        FD_ZERO(&set);
        FD_SET(sock, &set);
        tv.tv_sec = 1;
        tv.tv_usec = 500000;
        select(sock+1, &set, NULL, NULL, &tv);
        if (FD_ISSET(sock, &set)) {
            // Есть данные для чтения
            if ((recvStringlen = recvfrom(sock, recvString, RECV_NOTICE_SIZE, 0,
                    (struct sockaddr *) &udpAddrRecv, &udpAddrRecv_len)) < 0) {
                if (exitNotice(0) == 0) break;
            }
            recvString[strlen(recvString)] = '\0';
            if ((strcmp(recvString, ".") != 0) &&
                    (strcmp(recvString, " ") != 0)) { // Print Remind's
                if (mode == 1)
                    printNoticeGUI(recvString);
                else
                    printNoticeCLI(recvString);
            }
            if (exitNotice(0) == 0) { /* signal KILL -> exit from while*/
                break;
            }
            fail_time = 120;
        } else {
            printf("client connection failed. Check connection internet!\n");
            sleep(1);
            fail_time--;
            if (fail_time < 0)
                DieWithError("Connection failed. Droped.\n");
        }
    }
}

void FreeNotice(int id) {
    int sock;
    int err;
    struct Notice note;
    memset(&note, 0, sizeof (note));
    sock = CreateClientTCPSocket();

    note.time_begin = -2;
    note.time_period = id;
    if ((err = send(sock, &note, sizeof (note), 0)) < 0)
        DieWithError("send(), failed.");
    close(sock);

    return;
}

int CreateClientUDPSocket() {
    int sock; /* socket to create */
    struct sockaddr_in serverAddr; /* Local address */
    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct bind structure */
    memset(&serverAddr, 0, sizeof (serverAddr)); /* Zero out structure */
    serverAddr.sin_family = AF_INET; /* Internet address family */
    serverAddr.sin_port = htons(PORT); /* Server port */
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Bind to the broadcast port */
    if (0 > bind(sock, (struct sockaddr *) &serverAddr, sizeof (serverAddr)))
        DieWithError("bind failed().");

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

int getDisplayMode() {
    int n, mode;
    while (1) {
        printf("Select reminders display mode:\n \t1 - GUI\n\t2 - CLI\n");
        n = scanf("%d", &mode);
        if (n == 1 && (mode == 1 || mode == 2)) {

            break;
        }
        scanf("%*[^\n]"); // сброс буфера scanf
    }
    return mode;
}

void printNoticeGUI(char * str) {

    GtkWidget *label;
    GtkWidget *window;
    gtk_init(0, 0);
    //export NO_AT_BRIDGE=1
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Notice");
    label = gtk_label_new(str);
    gtk_container_add(GTK_CONTAINER(window), label);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
    gtk_widget_show_all(window);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_main();
}

void printNoticeCLI(char * str) {

    printf("Remind: %s\n", str);
}

int exitNotice(int exit) {
    static int stop = 1;
    if (exit == 1)
        stop--;

    return stop;
}

int getId(int id) {
    static int flag = 0;
    if (flag == 0)
        flag += id;

    return flag;
}

void DieWithError(const char *msg) {
    printf("Check your Internet connection. Try again!\n");
    perror(msg);
    exit(0);
}