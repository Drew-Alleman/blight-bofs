CC = x86_64-w64-mingw32-gcc
CFLAGS = -c -Wall

all: bofs/touch.x64.o bofs/test_bof.x64.o

bofs/touch.x64.o: src/touch/touch.c
	$(CC) $(CFLAGS) -I. $< -o $@

bofs/test_bof.x64.o: src/test_bof/test_bof.c
	$(CC) $(CFLAGS) -I. $< -o $@

clean:
	rm -f bofs/*.x64.o
