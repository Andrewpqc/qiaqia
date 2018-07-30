C = gcc
CC = g++
CFLAGS = -g -Wall -std=c++11
all: ./client/clientmain.cpp ./server/servermain.cpp server.o client.o csapp.o
	$(CC) $(CFLAGS) ./server/servermain.cpp server.o csapp.o -o qiaqia_server -lpthread
	$(CC) $(CFLAGS) ./client/clientmain.cpp client.o csapp.o -o qiaqia_client -lpthread

server: ./server/servermain.cpp server.o  csapp.o
	$(CC) $(CFLAGS) ./server/servermain.cpp server.o csapp.o -o qiaqia_server -lpthread

client: ./client/clientmain.cpp client.o csapp.o
	$(CC) $(CFLAGS) ./client/clientmain.cpp client.o csapp.o -o qiaqia_client -lpthread

server.o: ./server/server.cpp ./server/server.h 
	$(CC) $(CFLAGS) -c ./server/server.cpp

client.o: ./client/client.cpp ./client/client.h 
	$(CC) $(CFLAGS) -c ./client/client.cpp

csapp.o: ./csapp/csapp.c ./csapp/csapp.h
	$(C) -g -Wall -c ./csapp/csapp.c 

clean:
	rm -f *.o *.out qiaqia_server qiaqia_client 

install:
	sudo mv qiaqia_client qiaqia_server /usr/local/bin/