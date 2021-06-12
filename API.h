#ifndef API_H
#define API_H

#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "util.h"
#include "conn.h"

#define O_CREATE 1

int fd_c;


//static inline int openConnection(const char *sockname, int msec, const struct timespec abstime);

//         versione semplificata della openConnection()
static inline int simple_opneConnection(const char *sockname, int msec, int maxtime) {

    if (find_absolute_path((char*)sockname, NULL) == -1) {
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

// api_id = 2
static inline int closeConnection(const char *sockname) { // chiusura connessione col server
    //------- Parte di scrittura del messaggio -------

    if (find_absolute_path((char*)sockname, NULL) == -1) {
        return -1;
    }
    sendMessage_for_server(sockname, NULL, fd_c, "2 ");
    //------- Parte di lettura del messaggio -------
    msg_t *sms_read;
    int notused;
    CHECK_EXIT("calloc read", sms_read, calloc(1, sizeof(msg_t)), NULL)
    CHECK_EXIT("read ClConn size", notused, readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
    CHECK_EXIT("calloc read", sms_read->str, calloc(1, sms_read->len), NULL)
    CHECK_EXIT("read ClConn sms", notused, readn(fd_c, sms_read->str, sms_read->len), -1)
    long check_close = strtol(sms_read->str, (char **) NULL, 10);
    free(sms_read->str);
    free(sms_read);
    return (int) check_close;
}

// api_id = 3
static inline int openFile(const char *pathname, int flags) { // O_CREATE = 1 || only open = 0

    if (find_absolute_path((char*)pathname, NULL) == -1) {
        return -1;
    }
    if (flags == O_CREATE) { // nel caso della write
        // il server deve vedere se esiste di gia' il file, se si allora invia errore perche non posso creare qualcosa che gia esiste
        // se no allora il server crea il file e lo apre
        sendMessage_for_server("1",(char *) pathname, fd_c, "3 ");
        msg_t *sms_read;
        int notused;
        CHECK_EXIT("calloc read", sms_read, calloc(1, sizeof(msg_t)), NULL)
        CHECK_EXIT("read ClConn size", notused, readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
        CHECK_EXIT("calloc read", sms_read->str, calloc(1, sms_read->len), NULL)
        CHECK_EXIT("read ClConn sms", notused, readn(fd_c, sms_read->str, sms_read->len), -1)
        long check_close = strtol(sms_read->str, (char **) NULL, 10);
        free(sms_read->str);
        free(sms_read); // se check close est 0 allora a creato il file senza problemi
        return 0;
    } else {// per qualsiasi altra cosa non e' giusto
        return 0;
    }//Aperto alla lettura e alla scrittura.
    // Il file viene creato se non esiste, altrimenti viene troncato.
    // Il flusso è posizionato all'inizio del file.
}

// api_id = 4
static inline int readFile(const char *pathname, void ** buf, size_t * size) {
    // working progress
    return 0;
}

// api_id = 5
static inline int readNFiles(int N, const char* dirname) {
    // working progress
    return 0;
}
// api_id = 6
static inline int writeFile(const char *pathname, const char *dirname) {
    // working progress
    return 0;
}

#endif //API_H

