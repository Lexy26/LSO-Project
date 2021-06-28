#define _POSIX_C_SOURCE  200112L

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


#include "util_server.h"
#include "file_storage.h"
#include "util.h"
#include "conn.h"

// la funzione che i thread worker prendono quando vengono creati
void threadF(sms_arg * smsArg, info_storage_t ** storage) {
    printf("threadF activated : \n---------%s---------\n", smsArg->sms_info);

    if (smsArg->api_id == 1) { // api_id == 1 -> closeConnection
        char * tmp;
        char * token = strtok_r(smsArg->sms_info, ",",&tmp); // api_id + resto
        printf("~~~~~~ api_id 1 ~~~~~~\n");
        // token = sockname || tmp = NULL
        // Parte di scrittura del messaggio
        msg_t *sms_write;
        CHECK_RETURN("malloc sms_write", sms_write, malloc(sizeof(msg_t)), NULL)
        memset(sms_write, 0, sizeof(msg_t));
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

    } else if (smsArg->api_id == 2){ // api_id == 2 -> openFile -> api_id + pathname +  flag
        printf("~~~~~~ api_id 2 ~~~~~~\n");
        char * tmp;
        char * token = strtok_r(smsArg->sms_info, ",",&tmp); // token = pathname || tmp = flag
        printf("path : %s || flag : %s\n", token, tmp);
        char * pathfile = token;
        struct node * file_found;
        msg_t *sms_write = malloc(sizeof(msg_t));
        memset(sms_write, 0, sizeof(msg_t));
        int create = searchFileNode(token, *storage, &file_found);
        token = strtok_r(NULL, ",", &tmp);
        if ((create != -1 && strncmp(token, "1", 1) == 0) || (create == -1 && strncmp(token, "0", 1) == 0)) {
            sms_write->len = strlen("-1"); // errore
            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
            sms_write->str = (unsigned char *) "-1";
        } else {
            if (create == -1) {
                file_found = createFileNode(pathfile, smsArg->fd_client_id);
                insertFileNode(&file_found, storage, NULL);
            } else {
                while (1) {
                    //LOCK
                    if (file_found->fdClient_id == -1) { // vuol dire che e' chiuso
                        // quindi posso 'aprirlo' per questo client
                        file_found->fdClient_id = smsArg->fd_client_id;
                        break;
                    }
                    //UNLOCK
                }
            }
            sms_write->len = strlen("0");
            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
            sms_write->str = (unsigned char *) "0";
        }
        int notused;
        CHECK_EXIT("write ClConn size", notused, writen(smsArg->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
        CHECK_EXIT("write ClConn sms", notused, writen(smsArg->fd_client_id, sms_write->str, sms_write->len), -1)
        writen(smsArg->pipe_fd, &smsArg->fd_client_id, sizeof(int)); // reinserisco il client per la select
        printf("------------------ STORAGE 2 ------------------\n");
        printf("ram dispo    : %ld\n", (*storage)->ram_dispo);
        printf("n file dispo : %ld\n", (*storage)->nfile_dispo);
        printStorage(*storage);
        printf("-----------------------------------------------\n");

    } else if (smsArg->api_id == 3){ // api_id == 3 -> readFile
        printf("~~~~~~ api_id 3 ~~~~~~\n");
        char * tmp;
        char * token = strtok_r(smsArg->sms_info, ",",&tmp);
        printf("path : %s\n", token);
        struct node* file_read;
        searchFileNode(token, *storage, &file_read);
        // LOCK
        while(file_read->fdClient_id != smsArg->fd_client_id){}
        char size_char[BUFSIZE];
        sprintf(size_char, "%ld", file_read->file_sz);// okay 0 + size + buf
        sendMsg_ClientToServer_Append(smsArg->fd_client_id,"1,",file_read->pathname, size_char, file_read->init_pointer_file);
        // UNLOCK
        writen(smsArg->pipe_fd, &smsArg->fd_client_id, sizeof(int)); // reinserisco il client

    } else if (smsArg->api_id == 4){ // api_id == 4 -> readNFiles
        printf("~~~~~~ api_id 4 ~~~~~~\n");
        char * tmp;
        char * token = strtok_r(smsArg->sms_info, ",",&tmp); // token = N
        printf("N file da leggere : %s\n", token);
        long n_read = strtol(token, NULL, 10);
        if(n_read == 0) {
            n_read = (*storage)->nfile_dispo;
        }
        struct node * current = (*storage)->head;
        printf("n FINALE : %ld\n", n_read);
        while(n_read>0 && current != NULL){
            while(current->fdClient_id != -1){}// aspetto per un tempo determinato da sleep
            // se esco perche ho aspettato troppo allora questo file non lo inserisco, lo salto
            current->fdClient_id = smsArg->fd_client_id;
            // LOCK
            char size_char[BUFSIZE];
            sprintf(size_char, "%ld", current->file_sz);
            sendMsg_ClientToServer_Append(smsArg->fd_client_id,"1,",current->pathname, size_char, current->init_pointer_file);
            //UNLOCK
            current->fdClient_id = -1;
            current = current->son;
            --n_read;
        }
        unsigned char notused[4] = "000";
        sendMsg_ClientToServer_Append(smsArg->fd_client_id,"0,","00", "0", notused);//avverto che e' tutto finito
        writen(smsArg->pipe_fd, &smsArg->fd_client_id, sizeof(int)); // reinserisco il client

    } else if (smsArg->api_id == 5){ // api_id == 5 -> appendToFile
        printf("~~~~~~ api_id 5 ~~~~~~\n");
        char * tmp;
        char * token = strtok_r(smsArg->sms_info, ",",&tmp); // token = pathname
        struct node * file_node;
        if (searchFileNode(token, *storage, &file_node) == -1) {
            printf("non c'e'\n");
        }
        updateFileNode(smsArg->sms_content, smsArg->size_buf, &file_node, smsArg->fd_client_id);
        msg_t *sms_write = malloc(sizeof(msg_t));
        // parte da RIVEDERE : invio al client la lista dei file che espello
//        if (pathname_removed != NULL) {
//            sms_write->len = strlen("0,") + strlen(pathname_removed);
//            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
//            sms_write->str = (unsigned char *) "0,";
//            //strncat(sms_write->str,  pathname_removed, strlen(pathname_removed));
//        } else {
        sms_write->len = strlen("0");
        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
        sms_write->str = (unsigned char *) "0";
        int notused;
        CHECK_EXIT("write ClConn size", notused, writen(smsArg->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
        CHECK_EXIT("write ClConn sms", notused, writen(smsArg->fd_client_id, sms_write->str, sms_write->len), -1)
        printf("------------------ STORAGE 5 ------------------\n");
        printf("ram dispo    : %ld\n", (*storage)->ram_dispo);
        printf("n file dispo : %ld\n", (*storage)->nfile_dispo);
        printStorage(*storage);
        printf("MODIFICATO\n");
        printf("-----------------------------------------------\n");
        //free(sms_write->str);
        writen(smsArg->pipe_fd, &smsArg->fd_client_id, sizeof(int)); // reinserisco il client

    } else if (smsArg->api_id == 6){ // api_id == 6 -> closeFile
        printf("~~~~~~ api_id 6 ~~~~~~\n");
        struct node * file_node;
        searchFileNode(smsArg->sms_info, *storage, &file_node);

        if (file_node->fdClient_id == smsArg->fd_client_id) {
            file_node->fdClient_id = -1;
        }
        printf("chiusura file\n");
        writen(smsArg->pipe_fd, &smsArg->fd_client_id, sizeof(int)); // reinserisco il client
        printf("------------------ STORAGE 6 ------------------\n");
        printf("ram dispo    : %ld\n", (*storage)->ram_dispo);
        printf("n file dispo : %ld\n", (*storage)->nfile_dispo);
        printStorage(*storage);
        printf("-----------------------------------------------\n");
        msg_t *sms_write = malloc(sizeof(msg_t));
        sms_write->len = strlen("0");
        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
        sms_write->str = (unsigned char *) "0";
        int notused;
        CHECK_EXIT("write ClConn size", notused, writen(smsArg->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
        CHECK_EXIT("write ClConn sms", notused, writen(smsArg->fd_client_id, sms_write->str, sms_write->len), -1)
    }
}