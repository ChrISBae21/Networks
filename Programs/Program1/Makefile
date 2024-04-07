# Example makefile for CPE464 Trace program
#
# Just links in pcap library

CC = gcc
LIBS = -lpcap
CFLAGS = -g -Wall -pedantic
OBJFILES = main.c checksum.c ethernet.c ethernet.h ip.c ip.h
#CGLAGS = 

all:  trace

trace: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $(OBJFILES) $(LIBS)

clean:
	rm -f trace
