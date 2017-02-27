server:main.o threadpool.o server.o views.o
	gcc -pthread -o server main.o threadpool.o server.o views.o
main.o:src/server.h src/main.c
	gcc -c src/main.c
threadpool.o:src/threadpool.c src/threadpool.h
	gcc -pthread -c src/threadpool.c
server.o:src/server.c src/server.h
	gcc -pthread -c src/server.c
views.o:src/views.c src/views.h
	gcc -pthread -c src/views.c
clean:
	rm server main.o threadpool.o server.o views.o
