# Example makefile for CPE464 Trace program
#
# Just links in pcap library

CC = gcc
LIBS = -lpcap
CFLAGS = -g -Wall -pedantic 
#CGLAGS = 

all:  trace

trace: main.c checksum.c
	$(CC) $(CFLAGS) -o $@ main.c checksum.c $(LIBS)

clean:
	rm -f trace
