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

#define test 1

//extern pthread_mutex_t mutex;
extern queue_t * queue;
//extern info_storage_t storage;

void createQueue(queue_t ** pQueue) {
    (*pQueue)->count_rq = 0;
    (*pQueue)->head = NULL;
    (*pQueue)->last = NULL;
    printf("Queue create \n");
}

// insert element in queue
void push(struct sms_request** pSmsRequest) {
    pthread_mutex_lock(&queue->lock);
    if(queue->head == NULL && queue->last == NULL) { // unico nodo file nello storage
#if test == 1
        printf("INSERT request Queue\n");
#endif
        queue->head = *pSmsRequest;
        queue->last = *pSmsRequest;
    } else {
        struct sms_request *tmp_head = queue->head;
        queue->head = *pSmsRequest;//aggiorno la head ptr dello storage col nuovo file
        (*pSmsRequest)->son = tmp_head;
        tmp_head->father = *pSmsRequest;
    }
    queue->count_rq += 1;
    pthread_cond_signal(&queue->queue_cond);
    pthread_mutex_unlock(&queue->lock);
    //printf("PUSH fatto\n");
}


struct sms_request* pop() { //queue_t ** pQueue
    // remove e aggiorno last file
    struct sms_request * tmp_rq = queue->last;
#if test == 1
    printf("POP request Queue\n");
#endif
    if (tmp_rq->father != NULL) {
        struct sms_request * tmp_sms = tmp_rq->father;
        tmp_sms->son = NULL;
        queue->last = tmp_sms;
    } else {
        queue->head = NULL;
        queue->last = NULL;
    }
    queue->count_rq -= 1;
    return tmp_rq;
}

void printQueue() {
    int count = queue->count_rq;
    struct sms_request * current = queue->head;
    pthread_mutex_lock(&queue->lock);
    printf("********* QUEUE **********\n");
    while (count > 0) {
        fprintf(stderr,"fd client : %d\n", current->fd_client_id);
        current = current->son;
        count -= 1;
    }
    printf("**************************\n");
    pthread_mutex_unlock(&queue->lock);
}

// la funzione che i thread worker prendono quando vengono creati
void* threadF(void* args) {
    while(1) {
        //struct sms_request * smsRequest;
        printf("=============== Ritorna Thread Passivo %lu ===============\n", pthread_self());
        pthread_mutex_lock(&queue->lock);
        while (queue->head == NULL) {
            pthread_cond_wait(&queue->queue_cond, &queue->lock);
        }
        struct sms_request * request = pop();
        printf("__________________________________________________________\n");
        printf("threadF activated : %lu\nrequest: %s\n", pthread_self(), request->sms_info);
        printf("__________________________________________________________\n");
        pthread_mutex_unlock(&queue->lock);
        // ------------------ OKAY ------------------
        if (request->api_id == 1) { // api_id == 1 -> closeConnection
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // api_id + resto
#if test == 1
            printf("client prova a connettersi\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ api_id 1 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            // token = sockname || tmp = NULL
            // Parte di scrittura del messaggio
            msg_t *sms_write;
            CHECK_RETURN("malloc sms_write", sms_write, malloc(sizeof(msg_t)), NULL)
            memset(sms_write, 0, sizeof(msg_t));
            if (strncmp(token, SOCKNAME, strlen(SOCKNAME)) == 0) { // okay
                sms_write->len = strlen("0")+1;
                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                sms_write->str = (unsigned char *) "0";
            } else {
                sms_write->len = strlen("-1")+1; // errore
                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                sms_write->str = (unsigned char *) "-1";
            }
            int notused;
            printf("size close : %zu\n", sms_write->len);
            printf("contnent : %s\n", sms_write->str);
            CHECK_EXIT("write ClConn size", notused, writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EXIT("write ClConn sms", notused, writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
#if test == 1
            printf("Chiusura socket client : %d\n\n", request->fd_client_id);
            printf("client prova a connettersi\n");
            printf("------------------ STORAGE 1 ------------------\n");
            printf("ram dispo    : %ld\n", (*request->storage)->ram_dispo);
            printf("n file dispo : %ld\n", (*request->storage)->nfile_dispo);
            printStorage(request->storage);
            printf("-----------------------------------------------\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            free(sms_write);
            close(request->fd_client_id);
            // ------------------ OKAY ------------------
        } else if (request->api_id == 2){ // api_id == 2 -> openFile -> api_id + pathname +  flag
#if test == 1
            printf("client prova a connettersi\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ api_id 2 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            //printf("path : %s || flag : %s\n", token, tmp);
#endif
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // token = pathname || tmp = flag
            char * pathfile = token;
            struct node * file;
            msg_t *sms_write = malloc(sizeof(msg_t));
            memset(sms_write, 0, sizeof(msg_t));
            int create = searchFileNode(token, &(*request->storage), &file);
            token = strtok_r(NULL, ",", &tmp);
            if ((create != -1 && strncmp(token, "1", 1) == 0) || (create == -1 && strncmp(token, "0", 1) == 0)) {
                sms_write->len = strlen("-1"); // errore
                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                sms_write->str = (unsigned char *) "-1";
            } else {
                if (create == -1) {
                    char * buf_rm_path = malloc(sizeof(char));
                    memset(buf_rm_path, 0, sizeof(char));
                    file = createFileNode(pathfile, request->fd_client_id);
                    if (insertCreateFile(&file, &(*request->storage), &buf_rm_path)) {
                        sms_write->len = strlen("-1"); // errore
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                        sms_write->str = (unsigned char *) "-1";
                    } else {// preparo sms con path rimossi se ci sono
                        sms_write->len = strlen("0")+1;
                        if (buf_rm_path != NULL) {
                            sms_write->len += strlen(buf_rm_path)+1; // strlen(",") +
                        }
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
                        strncpy((char*) sms_write->str, "0", 2);
                        //sms_write->str = (unsigned char *) "0";
                        if (buf_rm_path != NULL) {
                            printf("insert okay : %s\n", buf_rm_path);
//                            strncat((char *) sms_write->str, ",", strlen(",")+1);
//                            printf("insert okay 2\n");
                            strncat((char *) sms_write->str, buf_rm_path, strlen(buf_rm_path));
                        }
                        printf("insert okay 3 : %s\n", sms_write->str);
                        // inserire, nel sms di invio, la lista dei path rimossi
                    }
                } else {// se file gia' ESTISTENTE, setto la var. fd_client_id
                    int cond = 1;
                    int cnt = 0;
                    while (cond && cnt < TIMER) {
#if TEST_LOCK == 1
                        printf("openFile 1 LOCK\n");
#endif
                        pthread_mutex_lock(&(*request->storage)->lock);
                        if (file->fdClient_id == -1) { // vuol dire che e' chiuso
                            // quindi posso 'aprirlo' per questo client
                            file->fdClient_id = request->fd_client_id;
                            cond = 0;
                        }
#if TEST_LOCK == 1
                        printf("openFile 1 UNLOCK\n");
#endif
                        pthread_mutex_unlock(&(*request->storage)->lock);
                        sleep((unsigned int) SLEEP_TIME);
                        ++cnt;
                    }
                    if(cond==1) {// vuol dire che file sempre occupato da un altro client
                        sms_write->len = strlen("-1"); // errore
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                        sms_write->str = (unsigned char *) "-1";
                    } else {
                        sms_write->len = strlen("0");
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
                        sms_write->str = (unsigned char *) "0";
                    }
                }
            }
            int notused;
            CHECK_EXIT("write openfile size", notused, writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EXIT("write openfile sms", notused, writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserisco il client per la select
#if test == 1
            printf("------------------ STORAGE 2 ------------------\n");
            printf("ram dispo    : %ld\n", (*request->storage)->ram_dispo);
            printf("n file dispo : %ld\n", (*request->storage)->nfile_dispo);
            printStorage(&(*request->storage));
            printf("-----------------------------------------------\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            // ------------------ OKAY ------------------
        } else if (request->api_id == 3){ // api_id == 3 -> readFile
#if test == 1
            printf("client prova a connettersi\n");
            printf("~~~~~~ api_id 3 ~~~~~~\n");
            //printf("path : %s\n", token);
#endif
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp);
            struct node* file_read;
            if (searchFileNode(token, &(*request->storage), &file_read) == -1) {
                unsigned char nnt[5] = "null";
                sendMsg_ClientToServer_Append(request->fd_client_id,"-1,",file_read->pathname, "5", nnt);
            } else {
                int cnt = 0; // aspetto che il file sia aperto dal client interessato
                while (file_read->fdClient_id != request->fd_client_id && cnt < TIMER) {
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
#if TEST_LOCK == 1
                printf("readFile LOCK\n");
#endif
                pthread_mutex_lock(&(*request->storage)->lock);
                if(file_read->fdClient_id != request->fd_client_id) {
                    unsigned char nnt[5] = "null";
                    sendMsg_ClientToServer_Append(request->fd_client_id, "-1,", file_read->pathname, "5", nnt);
                } else {
                    //while(file_read->fdClient_id != request->fd_client_id){}
                    char size_char[BUFSIZE];
                    sprintf(size_char, "%ld", file_read->file_sz);// okay 0 + size + buf
                    sendMsg_ClientToServer_Append(request->fd_client_id,"1,",file_read->pathname, size_char, file_read->init_pointer_file);

                }
#if TEST_LOCK == 1
                printf("readFile UNLOCK\n");
#endif
                pthread_mutex_unlock(&(*request->storage)->lock);
            }
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserisco il client
#if test
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            // ------------------ OKAY ------------------
        } else if (request->api_id == 4){ // api_id == 4 -> readNFiles
#if test == 1
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ api_id 4 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
            //printf("N file da leggere : %s\n", token);
#endif
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // token = N
            long n_read = strtol(token, NULL, 10);
            if(n_read == 0) {
                n_read = (*request->storage)->nfile_dispo;
            }
            struct node * current = (*request->storage)->head;
#if test == 1
            printf("client prova a connettersi\n");
            printf("n FINALE : %ld\n", n_read);
#endif
            while(n_read>0 && current != NULL){
#if TEST_LOCK == 1
                printf("readNFile LOCK\n");
#endif
                pthread_mutex_lock(&(*request->storage)->lock);
                if(current->modified == -1) { // se file non creato, vedo prossimo file
                    current = current->son;
                } else {
                    char size_char[BUFSIZE];
                    sprintf(size_char, "%ld", current->file_sz);
                    sendMsg_ClientToServer_Append(request->fd_client_id,"1,",current->pathname, size_char, current->init_pointer_file);
                    current->fdClient_id = -1;
                    current = current->son;
                    --n_read;
                }
#if TEST_LOCK == 1
                printf("readNFile UNLOCK\n");
#endif
                pthread_mutex_unlock(&(*request->storage)->lock);
            }
            if (n_read > 0) {
                unsigned char notused[5] = "null";
                sendMsg_ClientToServer_Append(request->fd_client_id,"-1,","00", "5", notused);//avverto che e' tutto finito
            } else {
                unsigned char notused[5] = "null";
                sendMsg_ClientToServer_Append(request->fd_client_id,"0,","00", "5", notused);//avverto che e' tutto finito
            }
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserisco il client

            // ------------------ OKAY ------------------
        } else if (request->api_id == 5){ // api_id == 5 -> appendToFile
#if test == 1
            printf("client prova a connettersi\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ api_id 5 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // token = pathname
            struct node * file_node;
            msg_t *sms_write = malloc(sizeof(msg_t));
            memset(sms_write, 0, sizeof(msg_t));
            if (searchFileNode(token, &(*request->storage), &file_node) == -1) {
                sms_write->len = strlen("-1"); // errore
                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                sms_write->str = (unsigned char *) "-1";
            } else {
                int cnt = 0; // aspetto che il file sia aperto dal client interessato
                while (file_node->fdClient_id != request->fd_client_id && cnt < TIMER) {
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
#if TEST_LOCK == 1
                printf("appendToFile LOCK\n");
#endif
                pthread_mutex_lock(&(*request->storage)->lock);
                if(file_node->fdClient_id != request->fd_client_id) {
                    sms_write->len = strlen("-1"); // errore
                    CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                    sms_write->str = (unsigned char *) "-1";
#if TEST_LOCK == 1
                    printf("appendToFile UNLOCK\n");
#endif
                    pthread_mutex_unlock(&(*request->storage)->lock);
                } else {
                    char * buf_rm_path = malloc(sizeof(char));
                    memset(buf_rm_path, 0, sizeof(char));
#if TEST_LOCK == 1
                    printf("appendToFile UNLOCK\n");
#endif
                    pthread_mutex_unlock(&(*request->storage)->lock);
                    // -------------------------NEW INSERT FILE------------------------------
                    if (UpdateFile(&file_node, &(*request->storage), &buf_rm_path, request->fd_client_id, request->size_buf, request->sms_content) == -1) {
                        sms_write->len = strlen("-1"); // errore
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                        sms_write->str = (unsigned char *) "-1";
                    } else {// preparo sms con path rimossi se ci sono
                        sms_write->len = strlen("0")+1;
                        if (buf_rm_path != NULL) {
                            sms_write->len += strlen(buf_rm_path)+1; // strlen(",") +
                        }
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
                        strncpy((char*) sms_write->str, "0", 2);
                        //sms_write->str = (unsigned char *) "0";
                        if (buf_rm_path != NULL) {
                            printf("insert okay : %s\n", buf_rm_path);
//                            strncat((char *) sms_write->str, ",", strlen(",")+1);
//                            printf("insert okay 2\n");
                            strncat((char *) sms_write->str, buf_rm_path, strlen(buf_rm_path));
                        }
                        printf("insert okay 3 : %s\n", sms_write->str);
                        // inserire, nel sms di invio, la lista dei path rimossi
                    }
                    //----------------------------------------------------------------------
                }
            }
            int notused;
            CHECK_EXIT("write ClConn size", notused, writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EXIT("write ClConn sms", notused, writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
#if test == 1
            printf("------------------ STORAGE 5 ------------------\n");
            printf("ram dispo    : %ld\n", (*request->storage)->ram_dispo);
            printf("n file dispo : %ld\n", (*request->storage)->nfile_dispo);
            printStorage(&(*request->storage));
            printf("MODIFICATO\n");
            printf("-----------------------------------------------\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            //free(sms_write->str);
            free(sms_write);
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserisco il client

            // ------------------ OKAY ------------------
        } else if (request->api_id == 6){ // api_id == 6 -> closeFile
#if test == 1
            printf("client prova a connettersi\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ api_id 6 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
            msg_t *sms_write = malloc(sizeof(msg_t));
            struct node * file_node;
            if(searchFileNode(request->sms_info, request->storage, &file_node) == -1) {
                sms_write->len = strlen("-1"); // errore
                CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                sms_write->str = (unsigned char *) "-1";
            } else {
                int cond = 1;
                int cnt = 0;
                while (cond && cnt < TIMER) {
                    pthread_mutex_lock(&(*request->storage)->lock);
                    if (file_node->fdClient_id == request->fd_client_id) {
                        // quindi posso 'aprirlo' per questo client
                        file_node->fdClient_id = -1;
                        cond = 0;
                    }
                    pthread_mutex_unlock(&(*request->storage)->lock);
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
                if(cond == 1) {
                    sms_write->len = strlen("-1"); // errore
                    CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                    sms_write->str = (unsigned char *) "-1";
                } else {
                    sms_write->len = strlen("0");
                    CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)), NULL)
                    sms_write->str = (unsigned char *) "0";
                }
            }
            int notused;
            CHECK_EXIT("write ClConn size", notused, writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EXIT("write ClConn sms", notused, writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
#if test == 1
            printf("chiusura file\n");
            printf("------------------ STORAGE 6 ------------------\n");
            printf("ram dispo    : %ld\n", (*request->storage)->ram_dispo);
            printf("n file dispo : %ld\n", (*request->storage)->nfile_dispo);
            printStorage(request->storage);
            printf("-----------------------------------------------\n");
            printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

#endif
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserisco il client
        }
    }

    return (void *) 0;
}