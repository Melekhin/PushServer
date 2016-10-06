#include "NoticeDay.h"

struct QueueNoticeDay * createQueueNoticeDay() {
    struct QueueNoticeDay *id_queue;
    id_queue = (struct QueueNoticeDay*) malloc(sizeof (struct QueueNoticeDay));
    id_queue->start = NULL;
    id_queue->tail = NULL;
    return id_queue;
}

void addNoticeDay(struct QueueNoticeDay *id_queue, char * text,
        long int time_sec_begin) {
    int i;
    struct NoticeDay *element = (struct NoticeDay *) malloc(sizeof (struct NoticeDay));
    for (i = 0; i < strlen(text); i++)
        element->text[i] = text[i];
    element->text[i] = '\0';
    element->time_sec_begin = time_sec_begin;
    element->next = NULL;
    if (id_queue->tail == NULL) {
        id_queue->start = element;
    } else {
        (id_queue->tail)->next = element;
    }
    id_queue->tail = element;
}

char * returnNoticeDay(struct QueueNoticeDay *id_queue, int time_sec) {
    char *str = NULL;
    int i;

    if (id_queue != NULL) {
        str = (char *) malloc(sizeof (char));
        str[0] = '\0';
        struct NoticeDay *step = id_queue->start;

        while (step != NULL) {
            if (time_sec == step->time_sec_begin) {
                int slen = strlen(step->text);
                int slen2 = strlen(str);
                char *strNotice = (char *) malloc(slen + slen2 + 1);
                for (i = 0; i < slen; i++)
                    strNotice[i] = step->text[i];
                for (i = 0; i < slen2; i++)
                    strNotice[slen + i] = str[i];
                strNotice[slen + slen2] = '\0';
                free(str);
                str = strNotice;
            }
            step = step->next;
        }
    }
    return str;
}

void freeQueueNoticeDay(struct QueueNoticeDay *id_queue) {
    if (id_queue != NULL) {
        struct NoticeDay *step;
        while (id_queue->start != NULL) {
            step = id_queue->start;
            id_queue->start = (id_queue->start)->next;
            free(step);
        }
        free(id_queue);
    }
    id_queue = NULL;
}

