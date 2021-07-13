# ------------ Makefile ---------------

# compilatore da usare
CC = gcc
# aggiungo i flags di compilazione
CCFLAGS += -std=c99 -Wall -g -Werror

# headers
INC = ./includes

# path per i socket name dei due test
SOCKETNAME = ./cs_sock
SOCKETNAME2 = ./cs_sock2

# dipendenze client e server
OBJ_CLIENT = $(INC)/util.o $(INC)/API.o $(INC)/util_client.o $(INC)/conn.o
OBJ_SERVER = $(INC)/file_storage.o $(INC)/util_server.o $(INC)/util.o $(INC)/conn.o $(INC)/configuration.o


server: server.c $(OBJ_SERVER)
	$(CC) $(CCFLAGS) server.c -I $(INC) $(OBJ_SERVER) -o server -lpthread

client: client.c $(OBJ_CLIENT)
	$(CC) $(CCFLAGS) client.c -I $(INC) $(OBJ_CLIENT) -o client

#  ============= Create object of header file ============  #

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

$(INC)/configuration.o: $(INC)/configuration.c $(INC)/configuration.h $(INC)/util.o
	$(CC) $(CCFLAGS) -c $(INC)/configuration.c -I $(INC) -o $@

# ===== Pulizia dei file generati attraverso test1 e test2 =====
cleanall: cleantest1 cleantest2

cleantest1:
	-rm -f includes/*.o server client output_server1.txt $(SOCKETNAME) logfile.txt test1

cleantest2:
	-rm -f includes/*.o server client
	-rm $(SOCKETNAME2) logfile2.txt test2 client1.txt client2.txt client3.txt


# ______________ TEST 1 ______________
test1: test1server test1client

test1server:
	make server
	valgrind --leak-check=full ./server -F config.txt > output_server1.txt 2>&1 &

test1client:
	make client
	bash ./test1.sh

# ______________ TEST 2 ______________
test2: test2server test2client

test2server:
	make server

test2client:
	make client
	bash ./test2.sh
