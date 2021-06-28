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

#include "configuration.h"
#include "util.h"
#include "conn.h"
#include "file_storage.h"
#include "util_server.h"


#define BUFSIZE 256


typedef struct {
    pthread_t thid;
    int var_libero; // 1 -> true; 0 -> false;
} thread_t;

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
    unlink(SOCKNAME);
    config_t *cfg = malloc(sizeof(config_t));
    configuration(argc, argv, &cfg);

    // create storage
    info_storage_t *storage;
    CHECK_EXIT("calloc storage", storage, calloc(1, sizeof(info_storage_t)), NULL)
    storage = createStorage(cfg->MEM_SIZE, cfg->N_FILE);

    /*------ THREAD WORKER INITIALIZER ------*/
    // (_______fare in seguito un controllo di thread W se occupato o no______)

    thread_t *lst_thread[cfg->N_THREAD]; // lista di thread worker
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        thread_t *th = malloc(sizeof(thread_t));
        if (pthread_create(&th->thid, NULL, threadW, NULL) != 0) {
            fprintf(stderr, "pthread_create FALLITA\n");
            exit(errno);
        }
        th->var_libero = 1; // thread libero
        lst_thread[i] = th;

    }
    //printf("creato i thread\n");


    int fd_server;
    if ((fd_server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {  // creo il socket
        perror("socket");
        exit(errno);
    }
    struct sockaddr_un server_address;    // setto l'indirizzo
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strncpy(server_address.sun_path, SOCKNAME, strlen(SOCKNAME) + 1);
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
        perror("pipe");
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

    while (1) {
        //printf("okayyyyy\n");
        printf("Waiting for connetions...\n");

        tmpset = set;// necessario perche' select modifica il master set
        if (select(fd_max + 1, &tmpset, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(errno);
        }
        for (int fd = 0; fd <= fd_max; ++fd) {
            //printf("fd in atto :: %d\n", fd);
            struct stat fd_stat;
            fstat(fd, &fd_stat);
            if (FD_ISSET(fd, &tmpset)) {
                int fd_client;
                if (fd == fd_server) {
                    fd_client = accept(fd_server, NULL, 0);
                    printf("Connected !\n");
                    FD_SET(fd_client, &set);// aggiunggo connection a master set
                    if (fd_client > fd_max) fd_max = fd_client;
                } else if (S_ISFIFO(fd_stat.st_mode)) {
                    //int leggi = 1;
                    int fd_client_read;
                    readn(fd, &fd_client_read, sizeof(int));
                    printf(" fd_client ritornato nella select letto : %d\n", fd_client_read);
                    // qui dovrei credo fare un while dove dovrei iterare il fdlst, per poi
                    // reinserire i client che stanno nella lista
                    FD_SET(fd_client_read, &set);
                    if (fd_client_read > fd_max) fd_max = fd_client_read;
                }//continue; MANCA controllo pipe, il quale lo faccio con una stat
                else { // se richiesta disponibile
                    printf("else\n");
                    FD_CLR(fd, &set);// lo tolgo dall'insieme dei descrittori
                    if (fd == fd_max) fd_max = updatemax(set, fd_max); // aggiorno il max

                    /*-------------- Request received --------------*/
                    // Parte di lettura del messaggio
                    unsigned char * sms;
                    printf("fd :%d\n", fd);
                    recievedMsg_ServerToClient(&sms, fd);
                    printf("contenuto del sms : ---- %s ----\n", sms);
                    sms_arg  * smsArg;
                    CHECK_EXIT("malloc smsArg", smsArg, malloc(sizeof(sms_arg)), NULL)
                    memset(smsArg, 0, sizeof(sms_arg));
                    char * tmp;
                    char * token = strtok_r((char *) sms, ",", &tmp); // api_id +resto
                    smsArg->api_id = strtol(token, NULL, 10);
                    smsArg->sms_info = tmp;
                    smsArg->fd_client_id = fd;
                    smsArg->pipe_fd = pipe_fd[1];
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
                    // Qui metto la richiesta nella coda che poi verra' data al thread
                    threadF(smsArg, &storage);

                    // Manca la parte della coda e dei thread worker

                    // devo rimettere il client a disposizione nel set della select, attraverso il file descr pipe


                }
                //printf("okay\n");
            }
        }

    }
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        pthread_join(lst_thread[i]->thid, NULL);
    }

    free(cfg);
    return 0;
}



