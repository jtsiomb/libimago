obj = src/main.o
bin = sconv

CC = gcc
CFLAGS = -pedantic -Wall -g -I../../src
LDFLAGS = ../../libimago.a -lpng -lz -ljpeg

$(bin): $(obj) ../../libimago.a
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
