src = $(wildcard src/*.c)
obj = $(src:.c=.o)
bin = glimago

CFLAGS = -pedantic -Wall -g -I../../src
LDFLAGS = ../../libimago.a -lGL -lglut -ldl -lpng -lz -ljpeg

$(bin): $(obj) ../../libimago.a
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
