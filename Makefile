CFLAGS=-Wall -Wextra -pedantic  -g
CPPFLAGS=-Iinclude/

all: gim

clean:
	rm -f gim
	rm -f build/*.o

build/%.o: src/%.c
	$(CC) -c $^ -o $@ $(CFLAGS) $(CPPFLAGS)

gim: build/tty.o build/buffer.o build/util.o build/main.o
	$(CC) $^ -o gim
