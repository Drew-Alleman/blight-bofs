CC = x86_64-w64-mingw32-gcc
CFLAGS = -c -Wall

BOFS = $(wildcard */src/*.c)
OBJS = $(foreach src,$(BOFS),$(dir $(src))../$(notdir $(src:.c=.x64.o)))

all: $(OBJS)

%/%.x64.o: %/src/%.c
	$(CC) $(CFLAGS) -I. $< -o $@

# Explicit targets per BOF
touch/touch.x64.o: touch/src/touch.c
	$(CC) $(CFLAGS) -I. $< -o $@

clean:
	find . -name "*.x64.o" -delete
