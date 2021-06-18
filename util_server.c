#define _POSIX_C_SOURCE  200112L

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


#include "util_server.h"
#include "util.h"
#include "conn.h"

// la funzione che i thread worker prendono quando vengono creati
void threadF(sms_arg * smsArg) {
    printf("threadF activated\n");
    char * tmp;
    char * token = strtok_r(smsArg->sms_info, ",",&tmp); // api_id + resto
    if (smsArg->api_id == 1) { // api_id == 1 -> closeConnection
        printf("api_id : 1\n");
        // token = sockname || tmp = NULL
        printf("token : %s\n", token);
        // Parte di scrittura del messaggio
        msg_t *sms_write;
        CHECK_RETURN("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
        if (strncmp(token, SOCKNAME, strlen(SOCKNAME)) == 0) { // okay
            sms_write->len = strlen("0");
            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
            sms_write->str = (unsigned char *) "0";
        } else {
            sms_write->len = strlen("-1"); // errore
            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
            sms_write->str = (unsigned char *) "-1";
        }
        int notused;
        CHECK_EXIT("write ClConn size", notused, writen(smsArg->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
        CHECK_EXIT("write ClConn sms", notused, writen(smsArg->fd_client_id, sms_write->str, sms_write->len), -1)
        printf("Chiusura socket client : %d\n\n", smsArg->fd_client_id);
        //free(sms_write->str);
        free(sms_write);
        close(smsArg->fd_client_id);

    } else if (smsArg->api_id == 2){ // api_id == 2 -> openFile
        printf("api_id 2\n");
        token = strtok_r(NULL, ",", &tmp);
        // UNDER CONSTRUCTION
        /*{
                        // O_CREATE va settato solo se c'e' una write
                        // open semplice solo nella lettura
                        token = strtok_r(NULL, " ", &tmp); // flag
                        int is_equal = strncmp(token, "1", 1);
                        token = strtok_r(NULL, " ", &tmp); // flag
                        long key_path = (long) token;
                        struct node * file;
                        CHECK_EXIT("calloc", file, calloc(1, sizeof(struct node)), NULL)
                        int find_file = searchFileNode(key_path, storage, &file);
                        msg_t *sms_write;
                        if ( is_equal == 0) { // flag = O_CREATE = 1
                            if (find_file == 0) { // file trovato
                                sms_write->len = strlen("-1"); // 3 = per il id della funzione dell'api
                                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                                memset(sms_write->str, '\0', sms_write->len);
                                sms_write->str = "-1";
                            } else {
                                // se non c'e' allora va bene e lo posso creare
                                FILE * new_file;
                                CHECK_EXIT("calloc new file", new_file, calloc(1, sizeof(FILE)), NULL)
                                new_file = fopen(token, "wb+");
                                createFileNode(key_path, new_file, );
                            }
                            // una volta creato va inserito nel strorage con insertFileNode()
                            // se tutto va bene, inviare un messaggio al client con la risposta di
                            // se e' andata tutto bene o no
                        } else {
                            // se il flag e' solo per aprire il file allora non c'e' bisogno di inserire nnt nello storage
                            // searchFileNode, se non c'e' riinvia la risposta di ritorno negativa
                            // prima di aprire il file deevo controllare la variabile is_open cosi evito di aprire
                            // qualcosa che e' gia' aperto
                            // Versione semplice : putacaso fosse gia aperto da un altro client allora ritorno perfetto
                            // se va tutto a buon fine allora devo aprire il file e settare la variabile
                            // is_open su 1 per avvertire che quel file est aperto
                            // per ora dico che e' aperto per tutti
                        }

                        msg_t *sms_write;
                        CHECK_RETURN("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
                        if (searchFileNode(key_path, storage, &file) == 0) { // find
                            sms_write->len = strlen("0");
                            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                            memset(sms_write->str, '\0', sms_write->len);
                            sms_write->str = "0";
                        } else { // guarad se il messaggio inviato al client posso riassumerlo in una funzione,
                            // per evitare di fare sempre copia e incolla
                            sms_write->len = strlen("-1"); // 3 = per il id della funzione dell'api
                            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                            memset(sms_write->str, '\0', sms_write->len);
                            sms_write->str = "-1";
                        }
                        CHECK_EXIT("write ClConn size", notused, writen(fd, &sms_write->len, sizeof(size_t)), -1)
                        CHECK_EXIT("write ClConn sms", notused, writen(fd, sms_write->str, sms_write->len), -1)
                        printf("okay\n");
                    }*/

    } else if (smsArg->api_id == 3){ // api_id == 3 -> readFile
        printf("api_id 3\n");
        token = strtok_r(NULL, ",", &tmp);


    } else if (smsArg->api_id == 4){ // api_id == 4 -> readNFiles
        printf("api_id 4\n");
        token = strtok_r(NULL, ",", &tmp);


    } else if (smsArg->api_id == 5){ // api_id == 5 -> appendToFile
        printf("api_id 5\n");
        token = strtok_r(NULL, ",", &tmp);


    } else if (smsArg->api_id == 6){ // api_id == 6 -> closeFile
        printf("api_id 6\n");
        token = strtok_r(NULL, ",", &tmp);


    }
}