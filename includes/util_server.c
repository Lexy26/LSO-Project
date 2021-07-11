/* ------------------         QUEUE         -------------------- */
/* ------------------ THREAD WORKER REQUEST -------------------- */
/* ------------------ THREAD WORKER SIGNAL  -------------------- */
#define _POSIX_C_SOURCE  200112L

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>


#include "util_server.h"
#include "file_storage.h"
#include "util.h"
#include "conn.h"


extern queue_t * queue;
extern statistics_t *statistics;
extern volatile int sigINT_sigQUIT;
extern volatile int sigHUP;
extern volatile int nclient;

char *strndup(const char *s, size_t n);

// initialise queue
void createQueue(queue_t ** pQueue) {
    (*pQueue)->count_rq = 0;
    (*pQueue)->head = NULL;
    (*pQueue)->last = NULL;
}

// push request in queue
void push(struct sms_request** pSmsRequest) {
    LOCK(&queue->lock, )
    // if there is no request in queue
    if(queue->head == NULL && queue->last == NULL) {
        queue->head = *pSmsRequest;
        queue->last = *pSmsRequest;
    } else {
        // insert from head
        struct sms_request *tmp_head = queue->head;
        queue->head = *pSmsRequest;
        (*pSmsRequest)->son = tmp_head;
        tmp_head->father = *pSmsRequest;
    }
    queue->count_rq += 1;// count number of request in queue
    if (pthread_cond_signal(&queue->queue_cond) != 0) {
        fprintf(stderr, "COND SIGNAL Error\n");
        return;
    }
    UNLOCK(&queue->lock, )
}

// pop from the end of queue
struct sms_request* pop() {
    struct sms_request * tmp_rq = queue->last;
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

// print all requests of queue
void printQueue() {
    int count = queue->count_rq;
    struct sms_request * current = queue->head;
    LOCK(&queue->lock, )
    fprintf(stderr,"************* QUEUE **************\n");
    while (count > 0) {
        fprintf(stderr,"- %s\n", current->sms_info);
        current = current->son;
        count -= 1;
    }
    fprintf(stderr,"**********************************\n");
    UNLOCK(&queue->lock, )
}
// this Thread Worker control dynamically signals
void * threadSignal(void * arg) {
    signalHandler_t * signalHandler = arg;
    sigset_t *set_signal = signalHandler->set_sig;
    int pipe_h = signalHandler->pipe_hup;
    int pipe_i_q = signalHandler->pipe_int_quit;
    while (1) {
        int sig;
        int s = sigwait(set_signal, &sig);
        if(s != 0) {
            perror("sigwait");
            exit(EXIT_FAILURE);
        }
        switch (sig) {
            case SIGHUP:
                fprintf(stderr, "Signal handling : SIGHUP\n");
                close(pipe_h);
                return (void *) 0;
            case SIGQUIT:
                fprintf(stderr, "Signal handling : SIGQUIT\n");
                close(pipe_i_q);
                return (void*) 0;
            case SIGINT:
                fprintf(stderr, "Signal handling : SIGINT\n");
                close(pipe_i_q);
                return (void*) 0;
            default:
                break;
        }
    }

}

// this Thread Worker execute request done by client
void* threadF(void* args) {
    // life cycle of thread
    while(1) {
        LOCK(&queue->lock, NULL)
        while (queue->head == NULL) {
            if (sigINT_sigQUIT == 1) {
                fprintf(stderr, "Closing Thread Worker [%lu]\n", pthread_self());
                UNLOCK(&queue->lock, NULL)

                return (void*) 0;
            }
            if(nclient == 0 && sigHUP == 1) {
                fprintf(stderr, "Closing Thread Worker [%lu]\n", pthread_self());
                UNLOCK(&queue->lock, NULL)
                return (void*) 0;
            }
            CHECK_NEQ_EXIT("COND WAIT Error", pthread_cond_wait(&queue->queue_cond, &queue->lock), 0)
        }
        struct sms_request * request = pop();
        UNLOCK(&queue->lock, NULL)
        msg_t *sms_write;
        CHECK_EXIT_VAR("malloc sms_write", sms_write, malloc(sizeof(msg_t)), NULL)
        memset(sms_write, 0, sizeof(msg_t));

        if (request->api_id == 1) {
            // ------------------ CLOSE CONNECTION ------------------
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp);
            // write answer to client about sockname client-server comparison
            if (strncmp(token, request->sockname, strlen(request->sockname)) == 0) { // okay
                sms_write->len = strlen("0");
                sms_write->str = (unsigned char *) strndup("0", strlen("0"));
            } else {
                sms_write->len = strlen("-1"); // errore
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
            }
            CHECK_EQ_EXIT("writen ClConn size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EQ_EXIT("writen ClConn sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
            // remove client from connected client set
            nclient -= 1;
            // close cllient connection
            fprintf(stderr, "Close connection\n");
            LOG_PRINT2_INT(request->logfile, "Close client connection", request->fd_client_id)
            close(request->fd_client_id);

        } else if (request->api_id == 2){
            // ------------------ OPEN FILE ------------------
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // token = pathname || tmp = flag
            char * pathfile = token;
            struct node * file;
            int create = searchFileNode(token, &(*request->storage), &file);

            token = strtok_r(NULL, ",", &tmp);
            if ((create != -1 && strncmp(token, "1", 1) == 0) || (create == -1 && strncmp(token, "0", 1) == 0)) {
                sms_write->len = strlen("-1"); // error
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
            } else {
                // this file doesn't exist storage, so create file node
                if (create == -1) {
                    // variable where there are files removed
                    char * buf_rm_path;
                    CHECK_EXIT_VAR("malloc buf_rm_path", buf_rm_path, malloc(sizeof(char)), NULL)
                    memset(buf_rm_path, 0, sizeof(char));
                    file = createFileNode(pathfile, request->fd_client_id);
                    if (insertCreateFile(&file, &(*request->storage), &buf_rm_path, &statistics->cnt_file_removed)) {
                        sms_write->len = strlen("-1"); // error
                        sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                    } else {
                        // prepare sms with removed files if any
                        sms_write->len = strlen("0");
                        if (buf_rm_path != NULL) {
                            sms_write->len += strlen(buf_rm_path)+1;
                        }
                        CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
                        strncpy((char*) sms_write->str, "0", 2);
                        if (buf_rm_path != NULL) {
                            strncat((char *) sms_write->str, buf_rm_path, strlen(buf_rm_path));
                        }
                        free(buf_rm_path);
                    }
                } else {
                    // if file exist, only modify the var. fd_client_id to indicate that this file is open
                    int cond = 1;
                    int cnt = 0;
                    while (cond && cnt < TIMER) {
                        LOCK(&(*request->storage)->lock, NULL)
                        if (file->fdClient_id == -1) {
                            // if file is closed (-1), this client can open it
                            file->fdClient_id = request->fd_client_id;
                            cond = 0;
                        }
                        UNLOCK(&(*request->storage)->lock, NULL)
                        sleep((unsigned int) SLEEP_TIME);
                        ++cnt;
                    }
                    if(cond==1) {// means that the file is always occupied by another client.
                        sms_write->len = strlen("-1"); // error
                        sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                    } else {
                        sms_write->len = strlen("0");
                        sms_write->str = (unsigned char *) strndup("0", strlen("0"));
                    }
                }
            }
            fprintf(stderr, "FINISH OPEN FILE\n");
            CHECK_EQ_EXIT("write openfile size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EQ_EXIT("write openfile sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)

        } else if (request->api_id == 3){
            // ------------------ READ FILE ------------------
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp);
            struct node* file_read;
            if (searchFileNode(token, &(*request->storage), &file_read) == -1) {
                // if the file is not found
                sms_write->len = strlen("-1"); // error
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                CHECK_EQ_EXIT("write openfile size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
                CHECK_EQ_EXIT("write openfile sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
            } else {
                int cnt = 0;
                // wait for the file to be opened by the client concerned
                while (file_read->fdClient_id != request->fd_client_id && cnt < TIMER) {
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
                LOCK(&(*request->storage)->lock, NULL)
                if(file_read->fdClient_id != request->fd_client_id) {
                    // if the client waited too long
                    sms_write->len = strlen("-1"); // error
                    sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                    CHECK_EQ_EXIT("write openfile size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
                    CHECK_EQ_EXIT("write openfile sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
                } else {
                    // write the content of the file to the client
                    char size_char[BUFSIZE];
                    sprintf(size_char, "%ld", file_read->file_sz);// 1 + size + buf
                    sendMsg_File_Content(request->fd_client_id,"1,",file_read->pathname, size_char, file_read->content_file);
                }
                UNLOCK(&(*request->storage)->lock, NULL)
            }
            fprintf(stderr, "FINISH READ FILE\n");


        } else if (request->api_id == 4){
            // ------------------ READ N FILES ------------------
            // checks the number of files to be read
            long n_read = strtol(request->sms_info, NULL, 10);
            if(n_read == 0) {
                n_read = (*request->storage)->nfile_dispo;
            }
            struct node * current = (*request->storage)->head;
            long check = n_read;
            // loop to read n_read files or all files in the storage
            while(n_read>0 && current != NULL){
                LOCK(&(*request->storage)->lock, NULL)
                if(current->modified == -1) {
                    // if the file has not yet been modified, go to the next file
                    current = current->son;
                } else {
                    // file available to be read
                    char size_char[BUFSIZE];
                    sprintf(size_char, "%ld", current->file_sz);
                    sendMsg_File_Content(request->fd_client_id,"1,",current->pathname, size_char, current->content_file);
                    current->fdClient_id = -1;
                    current = current->son;
                    --n_read;
                }
                UNLOCK(&(*request->storage)->lock, NULL)
            }
            if (n_read == check) {
                // no file has been read
                fprintf(stderr, "Nessuna lettura file\n");
                sms_write->len = strlen("-1"); // error
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
            } else {
                // all requested files have been read
                fprintf(stderr, "ciclo finito\n");
                sms_write->len = strlen("0"); // error
                sms_write->str = (unsigned char *) strndup("0", strlen("0"));
            }
            CHECK_EQ_EXIT("write openfile size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EQ_EXIT("write openfile sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
            fprintf(stderr, "FINISH READ N FILE\n");


        } else if (request->api_id == 5){
            // ------------------ APPEND TO FILE ------------------
            char * tmp;
            char * token = strtok_r(request->sms_info, ",",&tmp); // token = pathname
            struct node * file_node;
            // search file in storage
            if (searchFileNode(token, &(*request->storage), &file_node) == -1) {
                sms_write->len = strlen("-1"); // error
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
            } else {
                int cnt = 0;
                // wait for the file to be opened by the client concerned
                while (file_node->fdClient_id != request->fd_client_id && cnt < TIMER) {
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
                LOCK(&(*request->storage)->lock, NULL)
                if(file_node->fdClient_id != request->fd_client_id) {
                    sms_write->len = strlen("-1"); // error
                    sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                    UNLOCK(&(*request->storage)->lock, NULL)
                } else {
                    char * buf_rm_path;
                    CHECK_EXIT_VAR("malloc buf_rm_path", buf_rm_path, malloc(sizeof(char)), NULL)
                    memset(buf_rm_path, 0, sizeof(char));
                    UNLOCK(&(*request->storage)->lock, NULL)
                    // insert new file in storage
                    if (UpdateFile(&file_node, &(*request->storage), &buf_rm_path, request->fd_client_id, request->size_buf, request->sms_content, &statistics->cnt_file_removed) == -1) {
                        sms_write->len = strlen("-1"); // error
                        sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                    } else {
                        // prepare sms with removed files if any
                        sms_write->len = strlen("0");
                        if (buf_rm_path != NULL) {
                            sms_write->len += strlen(buf_rm_path)+1;
                        }
                        CHECK_EXIT_VAR("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(unsigned char)),NULL)
                        strncpy((char*) sms_write->str, "0", 2);
                        if (buf_rm_path != NULL) {
                            strncat((char *) sms_write->str, buf_rm_path, strlen(buf_rm_path));
                        }
                        free(buf_rm_path);
                        printf("insert okay 3 : %s\n", sms_write->str);
                    }
                }
            }
            // update max mem used
            long mem_usata = (*request->storage)->ram_tot - (*request->storage)->ram_dispo;
            if (mem_usata > statistics->max_mem_used) statistics->max_mem_used = mem_usata;
            // update max nfile pushed in storage
            long tot_nfile = (*request->storage)->nfile_tot - (*request->storage)->nfile_dispo;
            if (tot_nfile > statistics->max_nfile) statistics->max_nfile = tot_nfile;
            CHECK_EQ_EXIT("write ClConn size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EQ_EXIT("write ClConn sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
            fprintf(stderr, "APPEND FILE \n");


        } else if (request->api_id == 6){
            // ------------------ CLOSE FILE ------------------

            struct node * file_node;
            if(searchFileNode(request->sms_info, request->storage, &file_node) == -1) {
                sms_write->len = strlen("-1"); // error
                sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
            } else {
                int cond = 1;
                int cnt = 0;
                while (cond && cnt < TIMER) {
                    LOCK(&(*request->storage)->lock, NULL)
                    if (file_node->fdClient_id == request->fd_client_id) {
                        // if file is opened by concerned client, close it
                        file_node->fdClient_id = -1;
                        cond = 0;
                    }
                    UNLOCK(&(*request->storage)->lock, NULL)
                    sleep((unsigned int) SLEEP_TIME);
                    ++cnt;
                }
                if(cond == 1) {
                    sms_write->len = strlen("-1"); // error
                    sms_write->str = (unsigned char *) strndup("-1", strlen("-1"));
                } else {
                    sms_write->len = strlen("0");
                    sms_write->str = (unsigned char *) strndup("0", strlen("0"));

                }
            }
            CHECK_EQ_EXIT("write ClConn size", writen(request->fd_client_id, &sms_write->len, sizeof(size_t)), -1)
            CHECK_EQ_EXIT("write ClConn sms", writen(request->fd_client_id, sms_write->str, sms_write->len), -1)
        }
        if (request->api_id != 1)
            writen(request->pipe_fd, &request->fd_client_id, sizeof(int)); // reinserts client in the set for other request
        if (request->sms_content != NULL) free(request->sms_content);
        free(sms_write->str);
        free(sms_write);
        free(request);
    }
}