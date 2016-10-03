#include "NoticePeriod.h"

struct QueueNoticePeriod * createQueueNoticePeriod() {
    struct QueueNoticePeriod *id_queue;
    id_queue = (struct QueueNoticePeriod *) malloc(sizeof (struct QueueNoticePeriod));
    return id_queue;
}

void addNoticePeriod(struct QueueNoticePeriod *id_queue, char * text,
        long int time_sec_begin, int time_sec_period) {
    int i;
    struct NoticePeriod *element = (struct NoticePeriod *) malloc(sizeof (struct NoticePeriod));
    for (i = 0; i < strlen(text); i++)
        element->text[i] = text[i];
    element->text[i] = '\0';
    element->time_sec_begin = time_sec_begin;
    element->time_sec_period = time_sec_period;
    element->next = NULL;
    if (id_queue->tail == NULL) {
        id_queue->start = element;
    } else {
        (id_queue->tail)->next = element;
    }
    id_queue->tail = element;
}

char * returnNoticePeriod(struct QueueNoticePeriod *id_queue, int time_sec) {
    char *str = NULL;
    int i = 0;
    int number = 0;
    
    if (id_queue != NULL) {
        str = (char *) malloc(sizeof (char));
        str[0] = '\0';
        struct NoticePeriod *step = id_queue->start;
        while (step != NULL) {
            for (i = 0;; i++) { // different values time for single period notice
                number = step->time_sec_begin + i * step->time_sec_period;
                if (number == time_sec) {
                    int slen = strlen(step->text);
                    int slen2 = strlen(str);
                    char *strNotice = (char *) malloc(slen + slen2 + 1);
                    memset(strNotice, 0, slen + slen2 + 1);
                    strcat(strNotice, step->text);
                    strcat(strNotice, str);
                    free(str);
                    str = strNotice;
                } else if (number > time_sec) {
                    break;
                }
            }
            step = step->next;
        }
    }
    return str;
}

void freeQueueNoticePeriod(struct QueueNoticePeriod *id_queue) {
    struct NoticePeriod *step;
    while (id_queue->start != NULL) {
        step = id_queue->start;
        id_queue->start = (id_queue->start)->next;
        free(step);
    }
}
