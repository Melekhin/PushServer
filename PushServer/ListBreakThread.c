#include "ListBreakThread.h"

struct ListBreakThread * createListBreak() {
    struct ListBreakThread *listBreak;
    listBreak = (struct ListBreakThread*) malloc(sizeof (struct ListBreakThread));
    return listBreak;
}

int addConnectClient(struct ListBreakThread *listBreak) {
    struct ConnectClient *element = (struct ConnectClient *) malloc(sizeof (struct ConnectClient));
    element->id = addClientId(listBreak);
    element->exit = 0;
    element->next = NULL;
    if (listBreak->tail == NULL) {
        listBreak->start = element;
    } else {
        (listBreak->tail)->next = element;
    }
    listBreak->tail = element;
    return element->id;
}

int addClientId(struct ListBreakThread *listBreak) {
    int id = 1;
    struct ConnectClient *step = listBreak -> start;
    while (step != NULL) {
        id++;
        step = step -> next;
    }
    return id;
}

int clientBreak(struct ListBreakThread *listBreak, int id, int change) {
    struct ConnectClient *step = listBreak->start;
    while (step->id != id) {
        step = step->next;
    }
    step->exit += change;
    return step->exit;
}

void freeConnectClient(struct ListBreakThread *listBreak, int id) {
    struct ConnectClient *step = listBreak->start;
    if (step->id == id) { // delete first element list
        listBreak->start = listBreak->start->next;
        if (listBreak->start == NULL)
            listBreak->tail = NULL;
    } else if (listBreak->tail->id == id) { // delete tail element list
        while (step->next->next != NULL) {
            step = step->next;
        }
        listBreak->tail = step;
        listBreak->tail->next = NULL;
    } else {// delete element in entry list
        while (step->next->id != id) {
            step = step->next;
        }
        struct ConnectClient *step2 = step;
        step = step2->next;
        step2->next = step->next;
    }
    free(step);
    return;
}

void freeListBreak(struct ListBreakThread *listBreak) {
    struct ConnectClient *step;
    while (listBreak->start != NULL) {
        step = listBreak->start;
        listBreak->start = (listBreak->start)->next;
        free(step);
    }
}