#!/bin/sh 
make clean && make && gcc -O2 -Wall -I . -o tiny tiny.c csapp.o -lpthread
