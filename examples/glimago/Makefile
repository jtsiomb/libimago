src = $(wildcard src/*.c)
obj = $(src:.c=.o)
bin = glimago

CFLAGS = -pedantic -Wall -g
LDFLAGS = -limago -lGL -lglut

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
