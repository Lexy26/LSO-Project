#ifndef API_H
#define API_H
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/limits.h>
#include <errno.h>

#include "util.h"
#include "conn.h"

#define O_CREATE 1

int fd_c;

char *realpath(const char *path, char *resolved_path);

static inline int find_absolute_path(char* pathname){ // funzione di controllo path assoluto
    char *buf = pathname;
    char * abslotue_path;
    CHECK_EXIT("calloc", abslotue_path, calloc(PATH_MAX+1, sizeof(char)), NULL)
    abslotue_path = realpath(buf, NULL);
    if(abslotue_path == NULL || errno==ENOENT){
        printf("cannot find file with name[%s]\n", buf);
        free(abslotue_path);
        return -1;
    } else{
        printf("path[%s]\n", abslotue_path);
        free(abslotue_path);
        return 0;
    }
}

//static inline int openConnection(const char *sockname, int msec, const struct timespec abstime);

//         versione semplificata della openConnection()
static inline int simple_opneConnection(const char *sockname, int msec, int maxtime) {

    if (find_absolute_path((char*)sockname) == -1) {
        exit(EXIT_FAILURE);
    }
    CHECK_EXIT("socket", fd_c, socket(AF_UNIX, SOCK_STREAM, 0), -1)

    struct sockaddr_un serv_addr; // setto l'indirizzo
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, sockname, strlen(sockname) + 1);//do il nome al descr del socket per la connfd

    while (maxtime) { // ciclo con maxtime limite per aspettare una connessione col server
        // check del timer
        printf("client prova a connettersi\n");
        errno = 0;
        int connected;
        CHECK_RETURN("connect", connected, connect(fd_c, (struct sockaddr *) &serv_addr, sizeof(serv_addr)), -1)
        if (errno == ENOENT) {
            printf("waiting ...\n");
            maxtime--;
            if (maxtime > 0) sleep(msec);
        } else {
            //stampa valore del timer
            printf("client connesso\n");
            return fd_c;
        }
    }
    return -1;
}

// api_id = 1
static inline int closeConnection(const char *sockname) { // chiusura connessione col server
    //------- Parte di scrittura del messaggio -------

    if (find_absolute_path((char*)sockname) == -1) {
        exit(EXIT_FAILURE);
    }

    printf("sockname : %s\n", sockname);
    msg_t *sms_write;
    CHECK_EXIT("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
    sms_write->len = 3 + strlen(sockname); // 3 = per il id della funzione dell'api
    CHECK_EXIT("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
    memset(sms_write->str, '\0', sms_write->len);
    strncpy(sms_write->str, "2 ", 3);// api_id
    strncat(sms_write->str, sockname, strlen(sockname));
    int notused;
    CHECK_EXIT("write ClConn size", notused, writen(fd_c, &sms_write->len, sizeof(size_t)), -1)
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_write->str, sms_write->len), -1)
    free(sms_write->str);
    free(sms_write);
    printf("Richiesta chiusura socket client inviata...\n");
    //------- Parte di lettura del messaggio -------
    msg_t *sms_read;
    CHECK_EXIT("calloc read", sms_read, calloc(1, sizeof(msg_t)), NULL)
    CHECK_EXIT("read ClConn size", notused, readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
    CHECK_EXIT("calloc read", sms_read->str, calloc(1, sms_read->len), NULL)
    CHECK_EXIT("read ClConn sms", notused, readn(fd_c, sms_read->str, sms_read->len), -1)
    long close_ctrl = strtol(sms_read->str, (char **) NULL, 10);
    free(sms_read->str);
    free(sms_read);
    return (int) close_ctrl;
}

// api_id = 3
static inline int openFile(const char *pathname, int flags) {
    // working progress
}

// api_id = 4
static inline int readFile(const char *pathname, int flags) {
    // working progress
}

#endif //API_H

