CC = gcc
CFLAGS = -Wall -std=c99

all: mmu

mmu: mmu.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f mmu
