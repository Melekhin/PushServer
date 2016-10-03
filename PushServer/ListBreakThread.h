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

struct ListBreakThread * createListBreak();
int addConnectClient(struct ListBreakThread *listBreak);
int addClientId(struct ListBreakThread *listBreak);
int clientBreak(struct ListBreakThread *listBreak, int id, int change);
void freeConnectClient(struct ListBreakThread *listBreak, int id);
void freeListBreak(struct ListBreakThread *listBreak);
#endif /* LISTBREAKTHREAD_H */

