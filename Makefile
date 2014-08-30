all: cut_line

cut_line: cut_line.c
	gcc -o cut_line -O2 -std=gnu99 cut_line.c -lgd -ljpeg -lm -lpthread
