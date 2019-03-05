# Nickolas Gough, nvg081, 11181823

GCC = gcc
FLAGS = -g -std=gnu90 -Wall -pedantic


all: logger process


logger: logger.o common.o
	$(GCC) -o $@ $(FLAGS) $^

logger.o: logger.c
	$(CC) -o $@ -c $(FLAGS) $<


process: process.o common.o
	$(GCC) -o $@ $(FLAGS) $^

process.o: process.c
	$(CC) -o $@ -c $(FLAGS) $<


common.o: common.c common.h
	$(CC) -o $@ -c $(FLAGS) $<


clean:
	rm -rf logger process *.o
