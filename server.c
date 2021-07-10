/*-------------------FILE STORAGE SERVER-------------------*/

#define _POSIX_C_SOURCE  200112L
//#include <unistd.h>
//#include <sys/uio.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <signal.h>

#include "./includes/configuration.h"
#include "./includes/util.h"
#include "./includes/conn.h"
#include "./includes/file_storage.h"
#include "./includes/util_server.h"

#define test 0

queue_t *queue;
volatile int sigINT_sigQUIT = 0;
volatile int sigHUP = 0;
volatile int nclient = 0;
statistics_t *statistics;

int updatemax(fd_set set, int fdmax) {
    for (int i = (fdmax - 1); i >= 0; --i)
        if (FD_ISSET(i, &set)) return i;
    assert(1 == 0);
    //return -1;
}


int main(int argc, char *argv[]) {
    // create cfg to save configuration info for server init
    config_t *cfg;
    CHECK_EXIT_VAR("malloc cfg", cfg,malloc(sizeof(config_t)), NULL)
    configuration(argc, argv, &cfg);

    // Togliere quando faccio rm del socket nel makefile
    if(unlink(cfg->SOCKNAME)) {
        printf("non c'e' il socket\n");
    }
    // Create logfile to save internal operations of server
    FILE * logfile;
    CHECK_EXIT_VAR("fopen logfile", logfile,fopen(cfg->LOGFILE, "w"), NULL)
    fprintf(logfile, "------------------------------------------------\n");
    fprintf(logfile, "-          Internal Server Operations          -\n");
    fprintf(logfile, "------------------------------------------------\n\n");

    // create STORAGE
    info_storage_t *storage;
    CHECK_EXIT_VAR("calloc storage", storage, calloc(1, sizeof(info_storage_t)), NULL)
    storage = createStorage(cfg->MEM_SIZE, cfg->N_FILE);

    // create QUEUE Request
    CHECK_EXIT_VAR("calloc queue", queue, calloc(1, sizeof(queue_t)), NULL)
    createQueue(&queue);

    // init SIGNAL
    sigset_t set_signal;
    sigemptyset(&set_signal);
    sigaddset(&set_signal, SIGINT);
    sigaddset(&set_signal, SIGQUIT);
    sigaddset(&set_signal, SIGHUP);
    CHECK_NEQ_EXIT("pthread_sigmask", pthread_sigmask(SIG_BLOCK, &set_signal, NULL), 0)
    // pipe for signal soft close
    int pipe_sig[2];
    CHECK_NEQ_EXIT("pipe signal", pipe(pipe_sig), 0)
    // Create Signal Thread Worker
    signalHandler_t * signalHandler = malloc(sizeof(signalHandler_t));
    memset(signalHandler, 0, sizeof(signalHandler_t));
    signalHandler->set_sig = &set_signal;
    signalHandler->pipe = pipe_sig[1];
    pthread_t signal_thread;
    CHECK_NEQ_EXIT("pthread_create signal", pthread_create(&signal_thread, NULL, threadSignal, signalHandler), 0)

    //  THREAD WORKER INITIALIZER
    CHECK_NEQ_EXIT("pthread_cond_init", pthread_cond_init(&queue->queue_cond, NULL), 0)
    CHECK_NEQ_EXIT("pthread_mutex_init", pthread_mutex_init(&queue->lock, NULL), 0)
    pthread_t lst_thread[cfg->N_THREAD]; // thread worker list
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        pthread_t th;
        CHECK_NEQ_EXIT("pthread_create threadF",pthread_create(&th, NULL, threadF, NULL), 0)
        lst_thread[i] = th;
    }
    // Create socket for connection
    int fd_server;
    CHECK_EXIT_VAR("socket", fd_server, socket(AF_UNIX, SOCK_STREAM, 0), -1)
    struct sockaddr_un server_address; // set the address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, cfg->SOCKNAME, strlen(cfg->SOCKNAME) + 1);
    CHECK_NEQ_EXIT("bind", bind(fd_server, (struct sockaddr *) &server_address, sizeof(server_address)), 0)
    CHECK_NEQ_EXIT("listen", listen(fd_server, SOMAXCONN), 0)
    // Pipe for T.Manager and T.Worker communication
    int pipe_fd[2];
    CHECK_NEQ_EXIT("pipe fd", pipe(pipe_fd), 0)
    // set param for SELECT
    int fd_max = 0;
    fd_set set, tmpset;
    if (fd_server > fd_max) fd_max = fd_server;
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(fd_server, &set);// fd for new connection
    FD_SET(pipe_fd[0], &set);
    FD_SET(pipe_sig[0], &set);
    if (pipe_fd[0] > fd_max) fd_max = pipe_fd[0];
    if (pipe_sig[0] > fd_max) fd_max = pipe_sig[0];
    // set global statistic
    CHECK_EXIT_VAR("malloc statistics", statistics, malloc(sizeof(statistics_t)), NULL)
    memset(statistics, 0, sizeof(statistics_t));
    statistics->cnt_file_removed = 0;
    statistics->max_mem_used = 0;
    statistics->max_nfile = 0;
    // life cycle of server
    while (1) {
        if (sigINT_sigQUIT == 1) {
            LOG_PRINT(logfile, "SIGINT o SIGQUIT catched")
            break;
        }
        // when SIGHUP is catched and there is no client connection
        if(sigHUP == 1 && nclient == 0) {
            LOG_PRINT(logfile, "Server Shutdown...")
            break;
        } else {
            tmpset = set;
            CHECK_EQ_EXIT("select",select(fd_max + 1, &tmpset, NULL, NULL, NULL), -1)
            for (int fd = 0; fd <= fd_max; ++fd) {
                struct stat fd_stat;
                fstat(fd, &fd_stat);
                // fd ready to be used
                if (FD_ISSET(fd, &tmpset)) {
                    int fd_client;
                    // fd for new connection
                    if (fd == fd_server && sigHUP == 0) {
                        fd_client = accept(fd_server, NULL, 0);
                        LOG_PRINT2_INT(logfile, "new Client conncetion : ", fd_client)
                        FD_SET(fd_client, &set);// add new fd connection to "set"
                        if (fd_client > fd_max) fd_max = fd_client; // update fd max for "select" loop
                        nclient += 1; // +1 client connected to server
                    } else if (fd == pipe_fd[0]) {
                        int fd_buf;
                        read(fd, &fd_buf, sizeof(int));
                        LOG_PRINT2_INT(logfile, "client fd reactivated : ",fd_buf)
                        FD_SET(fd_buf, &set);
                        if (fd_buf > fd_max) fd_max = fd_buf;
                    }
                    else if (fd == pipe_sig[0]) {
                        if (sigHUP == 0) {
                            LOG_PRINT(logfile, "SIGHUP catched")
                            sigHUP = 1;
                        }
                    }
                    else { // client request available
                        FD_CLR(fd, &set);
                        if (fd == fd_max) fd_max = updatemax(set, fd_max); // update max
                        // Reading message request
                        unsigned char * sms;
                        recievedMsg(&sms, fd);
                        struct sms_request  * smsArg;
                        CHECK_EXIT_VAR("malloc smsArg", smsArg, malloc(sizeof(struct sms_request)), NULL)
                        memset(smsArg, 0, sizeof(struct sms_request));
                        // initialise message structure with useful info for thread worker
                        char * tmp;
                        char * token = strtok_r((char *) sms, ",", &tmp); // api_id
                        smsArg->api_id = strtol(token, NULL, 10);
                        smsArg->sms_info = tmp; // rest of message
                        smsArg->fd_client_id = fd;
                        smsArg->pipe_fd = pipe_fd[1];
                        smsArg->sockname = cfg->SOCKNAME;
                        smsArg->logfile = logfile;
                        if (smsArg->api_id ==  5) { // request : AppendToFile
                            token = strtok_r(NULL, ",", &tmp); // token = pathname, tmp = size
                            long sz = strtol(tmp, NULL, 10);
                            smsArg->size_buf = sz;
                            // content of file to append in storage
                            CHECK_EXIT_VAR("calloc smsArg->sms_content", smsArg->sms_content, calloc(sz+1, sizeof(unsigned char)), NULL)
                            CHECK_EQ_EXIT("readn smsArg->sms_content", readn(fd, smsArg->sms_content, sz), -1)
                        } else {
                            smsArg->sms_content = NULL;
                        }
                        smsArg->storage = &storage;
                        smsArg->son = NULL;
                        smsArg->father = NULL;
                        // insert message of client request in queue
                        push(&smsArg);
                    }
                }
            }
//            printQueue(queue); // to see the number of requests in queue
        }
    }
    CHECK_NEQ_EXIT("pthread_cond_broadcast", pthread_cond_broadcast(&(queue->queue_cond)), 0)
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        CHECK_NEQ_EXIT("pthread_join T.Worker", pthread_join(lst_thread[i], NULL), 0)
    }
    CHECK_NEQ_EXIT("signal_thread Signal", pthread_join(signal_thread, NULL), 0)
    // table with final statistics
    fprintf(logfile, "------------------------------------------------------------\n");
    fprintf(logfile, "-                        STATISTICS                        -\n");
    fprintf(logfile, "------------------------------------------------------------\n\n");
    float mem_used = (float) statistics->max_mem_used/(float) 1000000;
    fprintf(logfile, "Max size reached by storage    : %.6f Mbytes\n", mem_used);
    fprintf(logfile, "Max number of file in storage  : %ld\n", statistics->max_nfile);
    fprintf(logfile, "Number of file removed         : %d\n", statistics->cnt_file_removed);
    printStorage(&storage);
    fprintf(logfile, "\n------------------------------------------------------------\n");


    // close and free all
    // MANCA ROBA
    close(fd_server);
    fclose(logfile);
    free(statistics);
    free(storage);
    free(cfg->SOCKNAME);
    free(cfg->LOGFILE);
    free(cfg);
    return 0;
}



