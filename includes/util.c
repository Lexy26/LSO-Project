#define _POSIX_C_SOURCE  200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
//#include <limits.h>

#include "util.h"
#include "conn.h"

#define test 0

/* SCRITTO DAL CLIENT
 * struttura messaggio = dal client al server (Request):
 *                      API_ID     ARG1      ARG2          ARG3
 * - OpenFile        -> api_id + pathname +  flag
 * - AppendToFile    -> api_id + pathname + size_byte + content_file
 * - CloseFile       -> api_id + pathname
 * - CloseConnection -> api_id + sockname
 * - readFile        -> api_id + pathname
 * - readNFile       -> api_id +    N
 * */

/* LETTO DAL CLIENT
 * struttura messaggio = dal server al client (Risposta):
 *                         ARG1         ARG2        ARG3       ARG4
 * - OpenFile        -> -1 ko| 0 ok
 * - AppendToFile    -> -1 ko| 0 ok + file espulsi
 * - CloseFile       -> -1 ko| 0 ok
 * - CloseConnection -> -1 ko| 0 ok
 * - readFile        -> -1 ko| 0 ok + size_buf + content_buf
 * - readNFile       -> -1 ko| 0 ok + pathname + size_buf + content_buf
 * */

// -----------------------------  CLIENT  -----------------------------
//cambiare nome funzione
int sendMsg_ClientToServer_Append(int fd_c, char api_id[3], char *arg1, char *arg2, unsigned char *arg3) {
    sendMsg_ClientToServer(fd_c, api_id, arg1, arg2);
    msg_t *sms_content;
    int notused;
    CHECK_EXIT("calloc sms_write", sms_content, malloc(sizeof(msg_t)), NULL)
    memset(sms_content, 0, sizeof(msg_t));
    sms_content->len = strtol(arg2, NULL, 10);
    sms_content->str = calloc(sms_content->len, sizeof(unsigned char));
    memcpy(sms_content->str, arg3, sms_content->len);
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_content->str, sms_content->len), -1)
    free(sms_content->str);
    free(sms_content);
#if test == 1
    printf("Richiesta Append inviata...\n");
#endif
    return 0;
}

//cambiare nome funzione
int sendMsg_ClientToServer(int fd_c, char api_id[3], char *arg1, char *arg2) {
    msg_t *sms_write;
    CHECK_EXIT("calloc sms_write", sms_write, malloc(sizeof(msg_t)), NULL)
    memset(sms_write, 0, sizeof(msg_t));
    sms_write->len = strlen(api_id);
    if(arg1 != NULL) {
        sms_write->len += strlen(arg1);
        if (arg2 != NULL) sms_write->len += strlen(",");
    }
    if (arg2 != NULL) {
        sms_write->len += strlen(arg2);
    }
    CHECK_EXIT("calloc sms_write->str", sms_write->str, calloc(sms_write->len+2, sizeof(unsigned char)), NULL)
    // fare un print in esadecimale per vedere se ci sono rimasugli di
    strncpy((char *) sms_write->str, api_id, 3);
    if ( arg1 != NULL) {
        strncat((char *)sms_write->str, arg1, strlen(arg1));
        if (arg2 != NULL) strncat((char *)sms_write->str, ",", 2);
    }
    if (arg2 != NULL) {
        strncat((char *)sms_write->str, arg2, strlen(arg2));
    }
#if test == 1
    printf("contenuto : %s\n", sms_write->str);
#endif
    int notused;
    //printf("fd_c :: %d\n", fd_c);
    CHECK_EXIT("write ClConn size", notused, writen(fd_c, &sms_write->len, sizeof(size_t)), -1)
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_write->str, sms_write->len), -1)

    free(sms_write->str);
    free(sms_write);
#if test == 1
    printf("\nRichiesta send client inviata...\n");
#endif
    return 0;
}


//cambiare nome funzione
void recievedMsg_ServerToClient_Read(char ** pathname, unsigned char** sms_content, size_t *size_buf,int * check, int fd_c){
    // check + size + contenuto
//    printf("\n");
    unsigned char * sms_info;
    recievedMsg_ServerToClient(&sms_info, fd_c);
//    printf("sms read : %s\n", sms_info);

    char * tmp;
    char * token = strtok_r((char *)sms_info, ",", &tmp);// check

    if (strncmp(token, "1", 1)==0) { // okay, il contenuto di un file in arrivo dallo storage
//        printf("ce' anocra roba\n");
        msg_t * sms;
        CHECK_EXIT("calloc read", sms, malloc(sizeof(msg_t)), NULL)
        memset(sms, 0, sizeof(msg_t));
        token = strtok_r(NULL, ",", &tmp);
#if test == 1
        printf("path : %s\n", token);
#endif
        *pathname = token;
        token = strtok_r(NULL, ",", &tmp);
        int size_byte = strtol(token, NULL, 10);
        //printf("size : %s\n", token);
        *size_buf = size_byte;
        CHECK_EXIT("calloc sms", sms->str, calloc(size_byte, sizeof(unsigned char)), NULL)
        sms->len = size_byte;
        int n;
        CHECK_EXIT("read ClConn sms", n, readn(fd_c, sms->str, sms->len), -1)
//        printf("Arrivo del readn\n");
        //printf("read size byte : %zu\n", sms->len);
        *sms_content = sms->str;
        *check = 1;
        free(sms);
    } else if (strncmp(token, "0", 1)==0){ // non ci sono piu file da leggere
        printf("finito tutto in bellezza \n");
//        int n;
//        CHECK_EXIT("read ClConn sms", n, readn(fd_c, sms->str, sms->len), -1)
        *size_buf = 0;
        *sms_content = NULL;
        *check = 0;
        *pathname = NULL;
    } else { // errore, qualcosa e' andato storto
//        int n;
//        CHECK_EXIT("read ClConn sms", n, readn(fd_c, sms->str, sms->len), -1)
        printf("errore \n");
        *size_buf = 0;
        *sms_content = NULL;
        *check = -1;
        *pathname = NULL;
    }
    //free(sms_info);

}

//cambiare nome funzione
void recievedMsg_ServerToClient(unsigned char ** sms_info, int fd_c) {
    msg_t *sms_read;
    int notused;
    CHECK_EXIT("malloc rec server to client", sms_read, malloc(sizeof(msg_t)), NULL)
    memset(sms_read, 0, sizeof(msg_t));
    CHECK_EXIT("fd c len od msg", notused, readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
    //printf("size : %zu\n", sms_read->len);
    CHECK_RETURN("calloc rec server to client", sms_read->str, calloc(1, sms_read->len+1), NULL)
    CHECK_EXIT("read ClConn sms", notused, readn(fd_c, sms_read->str, sms_read->len), -1)
    *sms_info = sms_read->str;
    free(sms_read);
}


