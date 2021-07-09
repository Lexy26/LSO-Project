
#ifndef UTIL_SERVER_H
#define UTIL_SERVER_H

#include <pthread.h>

#include "file_storage.h"

// come e' strutturato il messaggio di arrivo per il server
struct sms_request{
    long api_id;
    long size_buf;
    char * sms_info;
    unsigned char * sms_content;
    int fd_client_id;
    int pipe_fd;
    char *sockname;
    info_storage_t ** storage;
    struct sms_request *son;
    struct sms_request *father;
};

typedef struct {
    int count_rq;
    struct sms_request *head;
    struct sms_request *last;
    pthread_cond_t queue_cond; // Cond variable
    pthread_mutex_t lock; // var per mutua esclusione
    int thread_occupato;
} queue_t;

typedef struct {
    int pipe;
    sigset_t  * set_sig;
}signalHandler_t;

typedef struct {
    long max_nfile;
    long max_mem_used;
    int cnt_file_removed;
}statistics_t;

void createQueue(queue_t ** pQueue);

void push( struct sms_request** pSmsRequest);

struct sms_request * pop();

void printQueue();

void * threadSignal(void * arg);

void* threadF(void * queue);

#endif //UTIL_SERVER_H
