server: server.o NoticeDay.o NoticePeriod.o ListBreakThread.o
	cc -g -std=c99 -Wall -o server server.o  NoticeDay.o \
	NoticePeriod.o ListBreakThread.o -pthread
server.o: server.c NoticeDay.h NoticePeriod.h ListBreakThread.h
	cc -g -std=c99 -Wall -c server.c

NoticeDay.o: NoticeDay.c NoticeDay.h
	cc -g -std=c99 -Wall -c NoticeDay.c
	
NoticePeriod.o: NoticePeriod.c NoticePeriod.h
	cc -g -std=c99 -Wall -c NoticePeriod.c

ListBreakThread.o: ListBreakThread.c ListBreakThread.h
	cc -g -std=c99 -Wall -c ListBreakThread.c

clean:
	rm server server.o NoticeDay.o NoticePeriod.o ListBreakThread.o
