# Makefile para Programacion Avanzada
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -msse2 -mavx2

TARGETS = rsa_gmp multiplicador128

all: $(TARGETS)

rsa_gmp: rsa_gmp.c
	$(CC) $(CFLAGS) -o $@ $< -lgmp

multiplicador128: multiplicador128.c
	$(CC) $(CFLAGS) -o $@ $<

run_multiplicador: multiplicador128
	./multiplicador128

run_rsa: rsa_gmp
	./rsa_gmp

clean:
	rm -f $(TARGETS) *.o

.PHONY: all run_multiplicador run_rsa clean
