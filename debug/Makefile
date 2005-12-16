INCLUDEDIR = -I. -I..
CC = gcc
CFLAGS = -g -Wall $(IPR_DEFINES)

all : $(patsubst %.c,%,$(wildcard *.c))

% : %.c
	$(CC) $(CFLAGS) $(INCLUDEDIR) -o $@ ../iprlib.o $< -lsysfs

clean:
	rm -f iprcache iprperf iprshutdown iprtest iprwritebuffer
