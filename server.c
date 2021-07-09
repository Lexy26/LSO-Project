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


#define BUFSIZE 256
#define test 0

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
queue_t *queue;
volatile int sigINT_sigQUIT = 0;
volatile int sigHUP = 0;
volatile int nclient = 0;
statistics_t *statistics;

//typedef struct {
//    pthread_t thid;
//    int var_libero; // 1 -> true; 0 -> false;
//} thread_t;

int updatemax(fd_set set, int fdmax) {
    for (int i = (fdmax - 1); i >= 0; --i)
        if (FD_ISSET(i, &set)) return i;
    assert(1 == 0);
    //return -1;
}

void *threadW() {
    //printf("collegato\n");
    return (void *) 0;
}


int main(int argc, char *argv[]) {

    config_t *cfg = malloc(sizeof(config_t));
    configuration(argc, argv, &cfg);

    if(unlink(cfg->SOCKNAME)) {
        printf("non c'e' il socket\n");
    }

    // create storage
    info_storage_t *storage;
    CHECK_EXIT("calloc storage", storage, calloc(1, sizeof(info_storage_t)), NULL)
    storage = createStorage(cfg->MEM_SIZE, cfg->N_FILE);

    // create Queue Request

    CHECK_EXIT("calloc storage", queue, calloc(1, sizeof(queue_t)), NULL)
    createQueue(&queue);

    pthread_cond_init(&queue->queue_cond, NULL);
    pthread_mutex_init(&queue->lock, NULL);

    // init signal SIGINT SIGQUIT SIGHUP
    sigset_t set_signal;
    sigemptyset(&set_signal);
    //il server termina il prima possibile, ossia non accetta più nuove richieste da parte dei
    //client connessi chiudendo tutte le connessioni attive (dovrà comunque generare il sunto
    // delle statistiche, descritto nel seguito)
    sigaddset(&set_signal, SIGINT);
    sigaddset(&set_signal, SIGQUIT);
    sigaddset(&set_signal, SIGHUP);
    if (pthread_sigmask(SIG_BLOCK, &set_signal, NULL) != 0) {
        perror("pthread sigmask");
        return -1;
    }
    // pipe for soft close
    int pipe_sig[2];
    if (pipe(pipe_sig) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    signalHandler_t * signalHandler = malloc(sizeof(signalHandler_t));
    memset(signalHandler, 0, sizeof(signalHandler_t));
    signalHandler->set_sig = &set_signal;
    signalHandler->pipe = pipe_sig[1];

    // Create Thread Worker Signal
    pthread_t signal_thread;
    if(pthread_create(&signal_thread, NULL, threadSignal, signalHandler) != 0) {
        perror("Creation Signal thread error");
        return -1;
    }


    /*------ THREAD WORKER INITIALIZER ------*/
    // (_______fare in seguito un controllo di thread W se occupato o no______)

    pthread_t lst_thread[cfg->N_THREAD]; // lista di thread worker
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        pthread_t th;
        if (pthread_create(&th, NULL, threadF, NULL) != 0) {
            fprintf(stderr, "pthread_create FALLITA\n");
            exit(errno);
        }
        lst_thread[i] = th;
    }

    int fd_server;
    if ((fd_server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {  // creo il socket
        perror("socket");
        exit(errno);
    }
    struct sockaddr_un server_address;    // setto l'indirizzo
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    printf("SOCKET Server : %s\n", cfg->SOCKNAME);
    strncpy(server_address.sun_path, cfg->SOCKNAME, strlen(cfg->SOCKNAME) + 1);
    if ((bind(fd_server, (struct sockaddr *) &server_address, sizeof(server_address))) !=
        0) {//assegno l'indirizzo al socket
        perror("bind");
        exit(errno);
    }
    if ((listen(fd_server, SOMAXCONN)) != 0) {//metto il socket in ascolto
        perror("listen");
        exit(errno);
    }
    int pipe_fd[2]; // qui creo la pipe
    if (pipe(pipe_fd) == -1) {
        perror("pipe fd");
        exit(EXIT_FAILURE);
    }
    int fd_max = 0;
    fd_set set, tmpset;
    if (fd_server > fd_max) fd_max = fd_server;
    // azzero sia il master set che il set temporaneo usato per la select
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(fd_server, &set);// sara' il descr. su cui faro' nuove connessioni, quindi accept()
    FD_SET(pipe_fd[0], &set); // per la lettura
    FD_SET(pipe_sig[0], &set);
    if (pipe_fd[0] > fd_max) fd_max = pipe_fd[0];
    if (pipe_sig[0] > fd_max) fd_max = pipe_sig[0];
    statistics = malloc(sizeof(statistics_t));
    memset(statistics, 0, sizeof(statistics_t));
    statistics->cnt_file_removed = 0;
    statistics->max_mem_used = 0;
    statistics->max_nfile = 0;
//    int serverGo = 1;
    while (1) {
        //printf("okayyyyy\n");
        if (sigINT_sigQUIT == 1) {
            printf("signal trap SIGINT o SIGQUIT 1\n");
            fflush(stdout);
            break;
        }
        if(sigHUP == 1 && nclient == 0) {
//            printf("***********************************************\n");
//            printf("********nclient : %d || sigHUP : %d *********\n", nclient, sigHUP);
//            printf("***********************************************\n");
            printf("signal trap SIGHUP\n");
            fflush(stdout);
            sigHUP = 1;
//            serverGo = 0;
            break;
        } else {
            printf("Waiting for connetions...\n");
            fflush(stdout);
            tmpset = set;// necessario perche' select modifica il master set
            if (select(fd_max + 1, &tmpset, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(errno);
            }
            for (int fd = 0; fd <= fd_max; ++fd) {
//                printf("fd in atto :: %d\n", fd);
                struct stat fd_stat;
                fstat(fd, &fd_stat);
                if (FD_ISSET(fd, &tmpset)) {
                    int fd_client;
                    if (fd == fd_server && sigHUP == 0) {
                        fd_client = accept(fd_server, NULL, 0);
                        printf("Connected ! %d\n", fd);
                        FD_SET(fd_client, &set);// aggiunggo connection a master set
                        if (fd_client > fd_max) fd_max = fd_client;
                        nclient += 1;
                    } else if (fd == pipe_fd[0]) {//(S_ISFIFO(fd_stat.st_mode)) {
                        //readn(fd, &size_fd_read, sizeof(unsigned long));
                        //char * fd_buf = calloc(size_fd_read, sizeof(char*));
                        int fd_buf;
                        read(fd, &fd_buf, sizeof(int));
                        fprintf(stderr,"fd_client ritornato nella select letto : %d\n", fd_buf);
                        // qui dovrei credo fare un while dove dovrei iterare il fdlst, per poi
                        // reinserire i client che stanno nella lista
                        FD_SET(fd_buf, &set);
                        if (fd_buf > fd_max) fd_max = fd_buf;
                    }
                    else if (fd == pipe_sig[0]) {
                        printf(" ----------- INIT CLOSE SIGHUP----------\n");
                        fflush(stdout);
                        sigHUP = 1;
                    }
                    else { // se richiesta disponibile
//                        printf("Request dispo\n");
                        FD_CLR(fd, &set);// lo tolgo dall'insieme dei descrittori
                        if (fd == fd_max) fd_max = updatemax(set, fd_max); // aggiorno il max
                        /*-------------- Request received --------------*/
                        // Parte di lettura del messaggio
                        unsigned char * sms;
                        recievedMsg_ServerToClient(&sms, fd);
#if test == 1
                        printf("fd :%d\n", fd);

#endif
                        fprintf(stderr,"contenuto del sms : ---- %s ----\n", sms);
                        struct sms_request  * smsArg;
                        CHECK_EXIT("malloc smsArg", smsArg, malloc(sizeof(struct sms_request)), NULL)
                        memset(smsArg, 0, sizeof(struct sms_request));
                        char * tmp;
                        char * token = strtok_r((char *) sms, ",", &tmp); // api_id +resto
                        smsArg->api_id = strtol(token, NULL, 10);
                        smsArg->sms_info = tmp;
                        smsArg->fd_client_id = fd;
                        smsArg->pipe_fd = pipe_fd[1];
                        smsArg->sockname = cfg->SOCKNAME;
                        if (smsArg->api_id ==  5) {
                            int n;
                            token = strtok_r(NULL, ",", &tmp); // token = pathname, tmp, size
                            long sz = strtol(tmp, NULL, 10);
                            smsArg->size_buf = sz;
                            CHECK_EXIT("malloc smsArg", smsArg->sms_content, calloc(sz+1, sizeof(unsigned char)), NULL) // problema con il +1 o altro
                            CHECK_EXIT("read api_id 5", n, readn(fd, smsArg->sms_content, sz), -1) // problema con il +1 o altro
                            //printf("sms content : %s", smsArg->sms_content);
                        } else {
                            smsArg->sms_content = NULL;
                        }
                        smsArg->storage = &storage;
                        smsArg->son = NULL;
                        smsArg->father = NULL;
                        push(&smsArg);
                    }
                }
            }
            printQueue(queue);
        }
    }
    printf("USCITA DAL WHILE\n");
    if (pthread_cond_broadcast(&(queue->queue_cond)) != 0) {
        // qui mettere controllo di unlock
        return -1;
    }
    printf(" BROADCAST Fatto\n");
    fflush(stdout);
    printf("cfg->N_THREAD : %d\n", cfg->N_THREAD);
    fflush(stdout);
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        printf("join thread : %d\n", i);
        if (pthread_join(lst_thread[i], NULL) != 0) {
            perror("pthread join");
            return -1;
        }
        fprintf(stderr, "join thread : %d\n", i);
    }
    pthread_join(signal_thread, NULL);
    fprintf(stderr,"--------------------------------------------\n");
    float mem_used = (float) statistics->max_mem_used/(float ) 1000000;
    fprintf(stderr, "num max mem usata : %.6f Mbytes\n", mem_used);
    fprintf(stderr, "num max mem usata : %ld bytes\n", statistics->max_mem_used);

    fprintf(stderr, "num max file inseriti : %ld\n", statistics->max_nfile);
    fprintf(stderr, "num file removed : %d\n", statistics->cnt_file_removed);
    fprintf(stderr,"-------------CHIUSURA server-----------------\n");
    printStorage(&storage);
    close(fd_server);
    free(statistics);
    free(storage);
    free(cfg);
    return 0;
}



