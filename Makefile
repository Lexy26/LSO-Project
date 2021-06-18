# ------------ Versione base del Makefile ---------------

# compilatore da usare
CC		=  gcc
# aggiungo alcuni flags di compilazione
CCFLAGS	        += -std=c99 -Wall -g -Werror
# gli include sono nella dir. corrente
# INCLUDES	= -I.

OBJ_CLIENT = util.o conn.o API.o configuration.o

OBJ_SERVER = file_storage.o util_server.o util.o conn.o configuration.o

server: server.c $(OBJ_SERVER)
	$(CC) $(CCFLAGS) server.c $(OBJ_SERVER) -o server -lpthread

client: client.c $(OBJ_CLIENT)
	$(CC) $(CCFLAGS) client.c $(OBJ_CLIENT) -o client

# ----------Per Debuggare------------
vos:
	valgrind ./server -F config.txt

voc:
	valgrind ./client -r pippo

outs:
	./server -F config.txt

outc:
	./client -r pippo
# -----------------------------------

file_storage.o: file_storage.c file_storage.h
	$(CC) $(CCFLAGS) -c file_storage.c

util_server.o: util_server.c util_server.h util.o conn.o
	$(CC) $(CCFLAGS) -c util_server.c

util.o: util.c util.h conn.o
	$(CC) $(CCFLAGS) -c util.c

conn.o: conn.c conn.h
	$(CC) $(CCFLAGS) -c conn.c

API.o: API.c API.h conn.o util.o
	$(CC) $(CCFLAGS) -c API.c

configuration.o: configuration.c configuration.h
	$(CC) $(CCFLAGS) -c configuration.c

clean: