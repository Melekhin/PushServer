#ifndef NOTICEPERIOD_H
#define NOTICEPERIOD_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SEND_NOTICE_SIZE 50

struct NoticePeriod {
    char text[SEND_NOTICE_SIZE];
    int time_sec_begin;
    int time_sec_period;
    struct NoticePeriod *next;
};

struct QueueNoticePeriod {
    struct NoticePeriod *start;
    struct NoticePeriod *tail;
};
/* Создание списка с периодескими напоминаниями */
struct QueueNoticePeriod * createQueueNoticePeriod();
/* Добавление напоминания в список */
void addNoticePeriod(struct QueueNoticePeriod *id_queue, char * text,
        long int time_sec_begin, int time_sec_period);
/* Получение текста напоминания, когда время пришло */
char * returnNoticePeriod(struct QueueNoticePeriod *id_queue, int time_sec);
/* Удаление всего списка напоминаний */
void freeQueueNoticePeriod(struct QueueNoticePeriod *id_queue);

#endif /* NOTICEPERIOD_H */

