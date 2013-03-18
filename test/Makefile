obj = test.o
bin = test

CC = gcc
CFLAGS = -pedantic -Wall -g -I../src
LDFLAGS = $(lib)

ifeq ($(shell uname -s), Darwin)
	lib = ../libimago.dylib
else
	lib = ../libimago.so.2.0
endif

$(bin): $(obj) $(lib)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
