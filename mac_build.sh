#!/bin/sh
gcc -c mac_src/main.c -o main.o
gcc -pthread -c mac_src/threadpool.c -o threadpool.o
gcc -pthread -c mac_src/server.c -o server.o
gcc -pthread -c mac_src/views.c -o views.o
gcc -pthread -o server main.o threadpool.o server.o views.o
rm main.o threadpool.o server.o views.o
