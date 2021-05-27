/*-------------------FILE STORAGE SERVER-------------------*/

#define _POSIX_C_SOURCE  200112L
//#include <unistd.h>
#include <sys/select.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "configuration.h"
#include <unistd.h>


#include <assert.h>
//#include "util.h"


#define SOCKNAME "./cs_sock"
#define BUFSIZE 256

typedef struct message {
    int len;
    char *str;
} msg_t;

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
    printf("collegato\n");
    return (void *) 0;
}


int main(int argc, char *argv[]) {
    config_t *cfg = malloc(sizeof(config_t));
    configuration(argc, argv, cfg);


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
    printf("creato i thread\n");


    int fd_server;
    if ((fd_server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {  // creo il socket
        perror("socket");
        exit(errno);
    }
    struct sockaddr_un server_address;    // setto l'indirizzo
    //memset(&server_address, '0', sizeof(server_address));
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

    int fd_max = 0;
    fd_set set, tmpset;
    if (fd_server > fd_max) fd_max = fd_server;
    // azzero sia il master set che il set temporaneo usato per la select
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(fd_server, &set);// sara' il descr. su cui faro' nuove connessioni, quindi accept()
    printf("Waiting for connetions...\n");
    while (1) {
        tmpset = set;// necessario perche' select modifica il master set
        if (select(fd_max + 1, &tmpset, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(errno);
        }
        for (int fd = 0; fd <= fd_max; ++fd) {
            if (FD_ISSET(fd, &tmpset)) {
                int fd_client;
                if (fd == fd_server) {
                    fd_client = accept(fd_server, NULL, 0);
                    printf("Connected !\n");
                    FD_SET(fd_client, &set);// aggiunggo connection a master set
                    if (fd_client > fd_max) fd_max = fd_client;
                }// else if (pipe) continue; MANCA controllo pipe
                else { // se richiesta disponibile
                    printf("else\n");
                    FD_CLR(fd, &set);// lo tolgo dall'insieme dei descrittori
                    if (fd == fd_max) fd_max = updatemax(set, fd_max); // aggiorno il max

                    /*------------SIMPLE CONNECTION with client INIT TEST-----------*/

                    msg_t messaggio;
                    messaggio.str = malloc(sizeof(char *)*BUFSIZE);
                    int sz;
                    char buffer[BUFSIZE];

                    printf("from Server\n");

                    read(fd, &sz, sizeof(int));
                    read(fd, buffer, sz);
                    printf("message ricieve : %s\n", buffer);

                    messaggio.str = "collegamento okay \n";
                    messaggio.len = (int) strlen(messaggio.str);
                    write(fd, &messaggio.len, sizeof(int));
                    write(fd, messaggio.str, messaggio.len);
                    printf("message wirtten to client\n");
                    // Manca la parte della coda e dei thread worker

                    // devo rimettere il client a disposizione nel manager


                    /*------------SIMPLE CONNECTION with client FINE-----------*/

                }
            }
        }

    }
    for (int i = 0; i < cfg->N_THREAD; ++i) {
        pthread_join(lst_thread[i]->thid, NULL);
    }

    free(cfg);
    return 0;
}
