
#ifndef UTIL_SERVER_H
#define UTIL_SERVER_H

#include <pthread.h>

#include "file_storage.h"

// how the arrival message for the server is structured
struct sms_request{
    long api_id; // identify the type of operation
    long size_buf; // dimension of file content if it will be given.
    char * sms_info; // this variable can contain : flag, path, socket, n
    unsigned char * sms_content; // is content of file, otherwise is null
    int fd_client_id; // fd of client who make this request
    int pipe_fd; // fd on which to write the client id, after executing the request
    char *sockname; // name of socket
    FILE * logfile; // log file, to print information inside it
    info_storage_t ** storage;
    struct sms_request *son; // element required for the queue
    struct sms_request *father; //element required for the queue
};

// structure of queue
typedef struct {
    int count_rq; // count number of request in the queue
    struct sms_request *head; // is the head pointer of queue
    struct sms_request *last; // is a pointer for the last element of queue
    pthread_cond_t queue_cond; // Cond variable
    pthread_mutex_t lock; // var for mutual exclusion
} queue_t;

// structure of variable given to Signal Thread Worker
typedef struct {
    int pipe_hup; // fd for sighup signal
    int pipe_int_quit; // fd for sigint and sigquit signal
    sigset_t  * set_sig; // set containing the signals concerned
}signalHandler_t;

// structure that contains general statistics
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
