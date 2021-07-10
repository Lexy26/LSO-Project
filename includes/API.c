/*      ------      API      ------      */

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

#define test 0

int fd_c;

//static inline int openConnection(const char *sockname, int msec, const struct timespec abstime);

//         versione semplificata della openConnection()
int simple_opneConnection(const char *sockname, int msec, int maxtime) {
    CHECK_EXIT_VAR("socket", fd_c, socket(AF_UNIX, SOCK_STREAM, 0), -1)
    struct sockaddr_un serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, sockname, strlen(sockname) + 1);
    while (maxtime) { // ciclo con maxtime limite per aspettare una connessione col server
        // check del timer
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

// api_id = 1
int closeConnection(const char *sockname) {
    char api_id[3] = "1,";
    sendMsg(fd_c, api_id, (char *) sockname, NULL);
    unsigned char * check_str;
    recievedMsg(&check_str, fd_c);
    int check_int = (int) strtol((char*) check_str,  NULL, 10);
    free(check_str);
    close(fd_c);
    printf("socket chiuso\n");
    return check_int;
}

// api_id = 2
int openFile(const char *pathname, int flags) { // O_CREATE = 1 || O_OPEN = 0
    char api_id[3];
    strncpy(api_id, "2,", 3);
    if (flags == O_CREATE) {
        char flagcreate[2];
        strncpy(flagcreate, "1", 2);
        sendMsg(fd_c, api_id, (char *) pathname, flagcreate);
    } else if(flags == O_OPEN) {
        char flagopen[2] = "0";
        strncpy(flagopen, "0", 2);
        sendMsg(fd_c, api_id, (char *) pathname, flagopen);
    }
    unsigned char * sms_read;
    recievedMsg(&sms_read, fd_c);
    int check_int = (int) strtol((char *) sms_read, NULL, 10);
    free(sms_read);
    return (int) check_int;
}

// api_id = 3
int readFile(const char *pathname, void ** buf, size_t * size) {
    char api_id[3] = "3,";
    sendMsg(fd_c, api_id, (char *) pathname, NULL);
    int check;
    char * path;
    receivedMsg_File_Content(&path,(unsigned char **) buf, size, &check, fd_c);
    if (check == 0 || check == 1) {
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
    sendMsg(fd_c, api_id, n, NULL);
    // loop that wait to receive n file or more
    printf("\n------------------------------------------------------\n");
    printf("-                      FILE READ                     -\n");
    printf("------------------------------------------------------\n\n");
    while (1) {
        unsigned char * sms_content;
        char * path;
        size_t size_buf;
        int check;
        receivedMsg_File_Content(&path, &sms_content, &size_buf, &check, fd_c);
        if (path != NULL) {
            printf("- %s\n", path);
        }
        if (dirname != NULL && check == 1) {
            // insert file in directory
            char *base = basename(path);
            char * filename;
            CHECK_EXIT_VAR("malloc", filename, malloc((strlen(dirname) +2+ strlen(base))* sizeof(unsigned char)), NULL)
            memset(filename, 0,(strlen(dirname) +2+ strlen(base)));
            strncpy(filename, dirname, strlen(dirname));
            strncat(filename, "/", strlen("/")+1);
            strncat(filename, base, strlen(base)+1);
            FILE *f;
            CHECK_EXIT_VAR("fopen dirname", f, fopen(filename, "wb"), NULL)
            CHECK_EQ_EXIT("fwrite f", fwrite(sms_content, size_buf, 1, f), -1)
            fclose(f);
            free(sms_content);
        } else if (check == 0){// there is no more file o read
            printf("------------------------------------------------------\n\n");
            return 0;
        } else if(check == -1) { //there is a problem when rreading file
            // settare errno ooprtunamente con una macro
            printf("------------------------------------------------------\n\n");
            return -1;
        }
        //free(path);
    }
}

// api_id = 5
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname) {
    char api_id[3] = "5,";
    char size_char[BUFSIZE];
    sprintf(size_char, "%zu", size);
    sendMsg_File_Content(fd_c, api_id, (char *) pathname, size_char, buf);
    unsigned char * sms_recieved;
    // content of message : check + list of file removed
    recievedMsg(&sms_recieved, fd_c);
    char * tmp;
    char * token = strtok_r((char *) sms_recieved, ",", &tmp);
    size_t check = strtol(token, NULL, 10);
    if (check == 0) {
        // check if there are no file removed
        token = strtok_r(NULL, ",", &tmp);
        if(token != NULL) {
            printf("\n------------------------------------------------------\n");
            printf("-                    FILE ESPULSI                    -\n");
            printf("------------------------------------------------------\n\n");
            while (token) {
                printf(" - %s\n", token);
                token = strtok_r(NULL, ",", &tmp);
            }
            printf("\n------------------------------------------------------\n\n");
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
    sendMsg(fd_c, api_id, (char *) pathname, NULL);
    unsigned char * sms;
    recievedMsg(&sms, fd_c);
    size_t check = strtol((char *) sms, NULL, 10);
    if (check == -1) {
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
    printf("Functionality not supported\n");
    return -1;
}

int writeFile(const char *pathname, const char *dirname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Functionality not supported\n");
    return -1;
}

int lockFile(const char* pathname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Functionality not supported\n");
    return -1;
}

int unlockFile(const char* pathname) {
    errno = ENOSYS; // Function not implemented (POSIX.1-2001).
    printf("Functionality not supported\n");
    return -1;
}





