CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=gnu11

all: parent child1 child2

parent: parent.c
	$(CC) $(CFLAGS) -o $@ $<

child1: child1.c
	$(CC) $(CFLAGS) -o $@ $<

child2: child2.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f parent child1 child2
