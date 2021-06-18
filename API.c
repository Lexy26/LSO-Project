#define _POSIX_C_SOURCE  200112L

#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>


#include "util.h"
#include "API.h"

int fd_c;

//static inline int openConnection(const char *sockname, int msec, const struct timespec abstime);

//         versione semplificata della openConnection()
int simple_opneConnection(const char *sockname, int msec, int maxtime) {
    // creo la connessione con il server
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
            if (maxtime > 0) sleep(msec/1000);
        } else {
            //stampa valore del timer
            printf("client connesso\n");
            return fd_c;
        }
    }
    return -1;
}

// api_id = 1 OKAY
int closeConnection(const char *sockname) { // chiusura connessione col server
    //------- Parte di scrittura del messaggio -------
    printf("Closing socket connection \n");
    char api_id[3] = "1,";
    sendMsg_ClientToServer(fd_c, api_id, (char *) sockname, NULL);
    //------- Parte di lettura del messaggio -------
    unsigned char * check_str;
    recievedMsg_ServerToClient(&check_str, fd_c);
    int check_int = (int) strtol((char*) check_str,  NULL, 10);
    free(check_str);
    close(fd_c);
    printf("socket chiuso\n");
    return check_int;
}

// api_id = 2 --- OpenFile -> api_id + pathname +  flag
int openFile(const char *pathname, int flags) { // O_CREATE = 1 || only open = 0
    char api_id[3] = "2,";
    if (flags == O_CREATE) {// creare file, se gia esiste errore
        // il server crea il file e lo apre
        char flagcreate[2] = "1";
        sendMsg_ClientToServer(fd_c, api_id, (char *) pathname, flagcreate);
    } else if(flags == O_OPEN) {// apre file
        char flagopen[2] = "0";
        sendMsg_ClientToServer(fd_c, api_id, (char *) pathname, flagopen);
    }
    //------- Parte di lettura del messaggio -------
    unsigned char * sms_read;
    recievedMsg_ServerToClient(&sms_read, fd_c);
    int check_int = (int) strtol((char *) sms_read, NULL, 10);
    free(sms_read); // se check close est 0 allora a creato il file senza problemi
    return (int) check_int;
}

// api_id = 3
int readFile(const char *pathname, void ** buf, size_t * size) {
    // invio la richiesta di lettura al server
    char api_id[3] = "3,";
    sendMsg_ClientToServer(fd_c, api_id, (char *) pathname, NULL);    // ricevo il msg con la size e il contenuto
    int check;
    recievedMsg_ServerToClient_Read(NULL,(unsigned char **) buf, size, &check, fd_c);
    if (check == 0) {
        return 0;
    } else {
        // settare errno
        return -1;
    }
}

// api_id = 4
int readNFiles(int N, const char* dirname) {
    char api_id[3] = "4,";
    char n[BUFSIZE];
    sprintf(n, "%d", N);
    // invio la richiesta di lettura al server
    sendMsg_ClientToServer(fd_c, api_id, n, NULL);    // ricevo il msg con la size e il contenuto
    int file_reading = 1;
    while (file_reading) {
        // ricevo il msg con la size e il contenuto
        unsigned char * sms_content;
        char * path;
        size_t size_buf;
        int check;
        recievedMsg_ServerToClient_Read(&path, &sms_content, &size_buf, &check, fd_c);
        if (dirname != NULL && check == 1) {
            // inserisco il file ricevuto dal server nella directory
            char *base = basename(path);
            strncat((char *)dirname, base, strlen(base)); // in modo da dire dove creare direttamente nella direcotry il file
            FILE *f = fopen(dirname, "wb"); // creo un file per scriverci dentro
            fwrite(sms_content, size_buf-1, 1, f);
            fclose(f);

        } else if (check == 0){
            file_reading = 0; // ovvero non ci sono piu file da leggere
        } else { //se c'e' stato un qualche errore
            // settare errno ooprtunamente con una macro
            return -1;
        }
        free(path);
        free(sms_content);
    }
    return 0;
}

// api_id = 5
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    char api_id[3] = "5,";
    sendMsg_ClientToServer_Append(fd_c, api_id, (char *) pathname,( char *) size, buf);
    // ricevo il msg con la size e il contenuto
    unsigned char * sms_recieved;
    recievedMsg_ServerToClient(&sms_recieved, fd_c); // check + lista pathname dei file espulsi

    char * tmp;
    char * token = strtok_r((char *) sms_recieved, ",", &tmp);
    size_t check = strtol(token, NULL, 10);
    if (check == 1) {
        token = strtok_r(NULL, ",", &tmp);
        printf("file espulsi : \n");
        while (token) {
            printf(" - Path File : %s\n", token);
            token = strtok_r(NULL, ",", &tmp);
        }
    } else {
        free(sms_recieved);
        return -1;
    }
    free(sms_recieved);
    return 0;
}

// api_id = 6
int closeFile(const char * pathname) {
    char api_id[3] = "6,";
    // invio la richiesta di lettura al server
    sendMsg_ClientToServer(fd_c, api_id, (char *) pathname, NULL);
    // ricevo il msg con la size e il contenuto -------------------------- da qui in poi
    unsigned char * sms;
    recievedMsg_ServerToClient(&sms, fd_c);
    size_t check = strtol((char *) sms, NULL, 10);
    if (check != -1) {
        // settare errno opportumanete
        free(sms);
        return -1;
    }
    free(sms);
    return 0;
}


// ------ FUNZIONI Opzionali ------

int removeFile(const char * pathname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Funzionalita' non supportata\n");
    return -1;
}

int writeFile(const char *pathname, const char *dirname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Funzionalita' non supportata\n");
    return -1;
}

int lockFile(const char* pathname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Funzionalita' non supportata\n");
    return -1;
}

int unlockFile(const char* pathname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Funzionalita' non supportata\n");
    return -1;
}





