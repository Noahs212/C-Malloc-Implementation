# Makefile for CS 2110 Homework 10, Spring 2022

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -Werror -Wextra -Werror=vla -Wstrict-prototypes -Wno-unused-function -Wold-style-definition -g
CFLAGS += -fcf-protection=none # Workaround for bug in GDB versions before 10.1, can be removed after GDB is updated
CHECK_LIBS = $(shell pkg-config --cflags --libs check)

# The C and H files
CFILES = my_malloc.c tests.c $(patsubst %.c,%.o,$(wildcard suites/*.c))
HFILES = my_malloc.h my_sbrk.h suites/suites.h
OFILES = $(patsubst %.c,%.o,$(CFILES))

.PHONY: run-tests run-gdb clean

tests: $(OFILES)
	$(CC) $(CFLAGS) $^ -o $@ $(CHECK_LIBS)

%.o: %.c $(HFILES)
	$(CC) $(CFLAGS) -c $< -o $@

run-tests: tests
	./tests $(TEST)

run-gdb: tests
	CK_FORK=no gdb --args ./tests $(TEST)

clean:
	rm -rf tests $(OFILES) *check*
