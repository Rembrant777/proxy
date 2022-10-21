#!/bin/sh 
make clean &&  gcc -g -Wall -c sbuf.c sbuf.h && make &&  gcc -g -Wall proxy.o cache.o csapp.o sbuf.o -o proxy -lpthread
