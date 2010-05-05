obj = test.o
bin = test

CC = gcc
CFLAGS = -pedantic -Wall -g -I../src
LDFLAGS = ../libimago.so.1.0

$(bin): $(obj) ../libimago.so.1.0
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
