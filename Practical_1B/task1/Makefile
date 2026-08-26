# Task 1: golden measure on the PC.
CC = gcc
CFLAGS = -O3 -march=native -Wall -Wextra -ffast-math
all: golden_measure
golden_measure: golden_measure.c
	$(CC) $(CFLAGS) -o $@ golden_measure.c -lm
run: golden_measure
	.\golden_measure.exe
check: run
clean:
	-del /Q golden_measure.exe 2>nul
	-del /Q golden_measure 2>nul
.PHONY: all run check clean