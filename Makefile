# ------------ Versione base del Makefile ---------------

# compilatore da usare
CC = gcc
# aggiungo alcuni flags di compilazione
CCFLAGS += -std=c99 -Wall -g -Werror

INC = ./includes
# dipendenze client e server
OBJ_CLIENT = $(INC)/util.o $(INC)/API.o $(INC)/util_client.o $(INC)/conn.o
OBJ_SERVER = $(INC)/file_storage.o $(INC)/util_server.o $(INC)/util.o $(INC)/conn.o $(INC)/configuration.o

#  ============= Create object of header file ============  #
server: server.c $(OBJ_SERVER)
	$(CC) $(CCFLAGS) server.c -I $(INC) $(OBJ_SERVER) -o server -lpthread

client: client.c $(OBJ_CLIENT)
	$(CC) $(CCFLAGS) client.c -I $(INC) $(OBJ_CLIENT) -o client


$(INC)/util_client.o: $(INC)/util_client.c $(INC)/util_client.h $(INC)/API.o
	$(CC) $(CCFLAGS) -c $(INC)/util_client.c -I $(INC) -o $@

$(INC)/file_storage.o: $(INC)/file_storage.c $(INC)/file_storage.h $(INC)/util.o
	$(CC) $(CCFLAGS) -c $(INC)/file_storage.c -I $(INC) -o $@

$(INC)/util_server.o: $(INC)/util_server.c $(INC)/util_server.h $(INC)/util.o $(INC)/conn.o $(INC)/file_storage.o
	$(CC) $(CCFLAGS) -c $(INC)/util_server.c -I $(INC) -o $@

$(INC)/util.o: $(INC)/util.c $(INC)/util.h $(INC)/conn.o
	$(CC) $(CCFLAGS) -c $(INC)/util.c -I $(INC) -o $@

$(INC)/conn.o: $(INC)/conn.c $(INC)/conn.h
	$(CC) $(CCFLAGS) -c $(INC)/conn.c -I $(INC) -o $@

$(INC)/API.o: $(INC)/API.c $(INC)/API.h $(INC)/util.o
	$(CC) $(CCFLAGS) -c $(INC)/API.c -I $(INC) -o $@

$(INC)/configuration.o: $(INC)/configuration.c $(INC)/configuration.h
	$(CC) $(CCFLAGS) -c $(INC)/configuration.c -I $(INC) -o $@

cleanall:
	-rm -f includes/*.o server client

# ______________ TEST 1 ______________
test1: test1server test1client
test1server:
	make server
	valgrind ./server -F config1.txt & # --leak-check=full
test1client:
	make client
	#chmod +x test1.sh
	bash ./test1.sh # chmod +x test1.sh || bash

# ______________ TEST 2 ______________
test2: test2server test2client
test2server:
	make server
test2client:
	make client
	bash ./test2.sh
