#ifndef NOTICEDAY_H
#define NOTICEDAY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SEND_NOTICE_SIZE 50

struct NoticeDay {
    char text[SEND_NOTICE_SIZE];
    int time_sec_begin;
    struct NoticeDay *next;
};

struct QueueNoticeDay {
    struct NoticeDay *start;
    struct NoticeDay *tail;
};
/* Создание списка с ежедневными напоминаниями */
struct QueueNoticeDay * createQueueNoticeDay();
/* Добавление напоминания в список */
void addNoticeDay(struct QueueNoticeDay *id_queue, char * text, 
        long int time_sec_begin);
/* Получение текста напоминания, когда время пришло */
char * returnNoticeDay(struct QueueNoticeDay *id_queue, int time_sec);
/* Удаление всего списка напоминаний */
void freeQueueNoticeDay(struct QueueNoticeDay *id_queue);
#endif /* NOTICEDAY_H */

