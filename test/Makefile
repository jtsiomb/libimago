obj = test.o
bin = test

CC = gcc
CFLAGS = -pedantic -Wall -g -I../src
LDFLAGS = ../libimago.a

$(bin): $(obj) ../libimago.a
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
