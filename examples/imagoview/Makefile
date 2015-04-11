src = $(wildcard src/*.c)
obj = $(src:.c=.o)
bin = imagoview

CFLAGS = -pedantic -Wall -g
LDFLAGS = -limago -lX11

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
