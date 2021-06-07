/*-------------------FILE STORAGE SERVER-------------------*/

//#define _POSIX_C_SOURCE  200112L
//#include <unistd.h>
//#include <ctype.h>
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
#include "configuration.h"
#include <unistd.h>
#include <assert.h>
#include "util.h"
#include "conn.h"


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

                    /*-------------- CLOSE CONNECTION Request --------------*/
                    // Parte di lettura del messaggio
                    int notused;
                    msg_t *sms_read;
                    CHECK_EXIT("calloc read", sms_read, calloc(1, sizeof(msg_t)), NULL)
                    CHECK_EXIT("read ClConn size", notused, readn(fd, &sms_read->len, sizeof(size_t)), -1)
                    CHECK_EXIT("calloc read", sms_read->str, calloc(1, sms_read->len), NULL)
                    CHECK_EXIT("read ClConn sms", notused, readn(fd, sms_read->str, sms_read->len), -1)
                    printf("message recieve : %s\n", sms_read->str);
                    //    TOKENIZZO i valori nella stringa per estrapolare le info inviate dal client
                    char *tmp;
                    char *token = strtok_r(sms_read->str, " ", &tmp);
                    long api_id = strtol(token, NULL, 10);
                    printf("api id : %ld\n", api_id);

                    // vedo a quale richiesta corrisponde attraverso l'api_id

                    if (api_id == 2) { // ovvero CloseConnection()
                        // QUI controlla che sockname corrisponda a quello con cui il client e' stato aperto
                        token = strtok_r(NULL, " ", &tmp);
                        printf("token : %s\n", token);
                        msg_t *sms_write;
                        CHECK_RETURN("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
                        if (strncmp(token, SOCKNAME, strlen(SOCKNAME)) == 0) {
                            sms_write->len = strlen("1");
                            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                            memset(sms_write->str, '\0', sms_write->len);
                            sms_write->str = "1";
                        } else {
                            sms_write->len = strlen("-1"); // 3 = per il id della funzione dell'api
                            CHECK_RETURN("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
                            memset(sms_write->str, '\0', sms_write->len);
                            sms_write->str = "-1";
                        }
                        // Parte di scrittura del messaggio
                        CHECK_EXIT("write ClConn size", notused, writen(fd, &sms_write->len, sizeof(size_t)), -1)
                        CHECK_EXIT("write ClConn sms", notused, writen(fd, sms_write->str, sms_write->len), -1)
                        printf("Chiusura socket client : %d\n\n", fd);
                        close(fd);
                        /*-------------- CLOSE CONNECTION Request FINE --------------*/

                    }

                    // Manca la parte della coda e dei thread worker

                    // devo rimettere il client a disposizione nel manager


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



