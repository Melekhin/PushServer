#ifndef LISTBREAKTHREAD_H
#define LISTBREAKTHREAD_H
#include <stdio.h>
#include <stdlib.h>

struct ConnectClient {
    int id;
    int exit;
    struct ConnectClient *next;
};

struct ListBreakThread {
    struct ConnectClient *start;
    struct ConnectClient *tail;
};

/* Создание списка клиетов */
struct ListBreakThread * createListBreak();
/* Добавление нового подсоединившегося клиента к серверу */
int addConnectClient(struct ListBreakThread *listBreak);
/* Добавление id клиента */
int addClientId(struct ListBreakThread *listBreak);
/* отключение клиента по id от сервера, по значению которое передается  change */
int clientBreak(struct ListBreakThread *listBreak, int id, int change);
/* Удалеине нужного нам клиента по его id */
void freeConnectClient(struct ListBreakThread *listBreak, int id); 
/* Удаление всего списка клиентов */
void freeListBreak(struct ListBreakThread *listBreak);
#endif /* LISTBREAKTHREAD_H */

