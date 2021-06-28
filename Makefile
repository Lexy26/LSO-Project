# ------------ Versione base del Makefile ---------------

# compilatore da usare
CC		=  gcc
# aggiungo alcuni flags di compilazione
CCFLAGS	        += -std=c99 -Wall -g -Werror
# gli include sono nella dir. corrente
# INCLUDES	= -I.

OBJ_CLIENT = util.o API.o util_client.o

OBJ_SERVER = file_storage.o util_server.o util.o conn.o configuration.o

server: server.c $(OBJ_SERVER)
	$(CC) $(CCFLAGS) server.c $(OBJ_SERVER) -o server -lpthread

client: client.c $(OBJ_CLIENT)
	$(CC) $(CCFLAGS) client.c $(OBJ_CLIENT) -o client

# ----------Per Debuggare------------
vos:
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./server -F config.txt

#-w /home/nessy/Desktop/Savetmp,n=5
#-W client.c,server.c
#-R n=0
#-d /home/nessy/CLionProjects/Progetto-LSO/YOLO -r client.c,server.c

voc:
	valgrind --leak-check=full ./client -W client.c,server.c -d /home/nessy/CLionProjects/Progetto-LSO/YOLO -R #  --track-origins=yes --show-leak-kinds=all

outs:
	./server -F config.txt

outc:
	./client -w /home/nessy/Desktop/Savetmp,n=0
# -----------------------------------

util_client.o: util_client.c util_client.h API.o
	$(CC) $(CCFLAGS) -c util_client.c


file_storage.o: file_storage.c file_storage.h util.o
	$(CC) $(CCFLAGS) -c file_storage.c

util_server.o: util_server.c util_server.h util.o conn.o file_storage.o
	$(CC) $(CCFLAGS) -c util_server.c

util.o: util.c util.h conn.o
	$(CC) $(CCFLAGS) -c util.c

conn.o: conn.c conn.h
	$(CC) $(CCFLAGS) -c conn.c

API.o: API.c API.h util.o
	$(CC) $(CCFLAGS) -c API.c

configuration.o: configuration.c configuration.h
	$(CC) $(CCFLAGS) -c configuration.c

clean: