CC = x86_64-w64-mingw32-gcc
CFLAGS = -c -Wall

all: touch/touch.x64.o

touch/touch.x64.o: touch/src/touch.c
	$(CC) $(CFLAGS) -I. $< -o $@

clean:
	find . -name "*.x64.o" -delete
