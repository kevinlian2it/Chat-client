CC = gcc
TARGETS = chatserver chatclient
CFLAGS = -g -Wall -Werror -pedantic-errors
LDFLAGS =

.PHONY: all clean

all: $(TARGETS)

chatserver: chatserver.c util.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ chatserver.c

chatclient: chatclient.c util.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ chatclient.c

clean:
	rm -f $(TARGETS)

