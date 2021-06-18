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



char *realpath(const char *path, char *resolved_path);


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

//            Questa funzione est specifica per l'API AppendToFile()
int sendMsg_ClientToServer_Append(int fd_c, char api_id[3], char *arg1, char *arg2, unsigned char *arg3) {
    sendMsg_ClientToServer(fd_c, api_id, arg1, arg2);
    msg_t *sms_content;
    int notused;
    CHECK_EXIT("calloc sms_write", sms_content, calloc(1, sizeof(msg_t)), NULL)
    sms_content->len = strtol(arg2, NULL, 10);
    memcpy(sms_content->str, arg3, sms_content->len);
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_content->str, sms_content->len), -1)
    free(sms_content->str);
    free(sms_content);
    printf("Richiesta chiusura socket client inviata...\n");
    return 0;
}


int sendMsg_ClientToServer(int fd_c, char api_id[3], char *arg1, char *arg2) {
    msg_t *sms_write;
    CHECK_EXIT("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
    sms_write->len = 3 + strlen(arg1);
    if (arg2 != NULL) {
        sms_write->len += strlen(",") + strlen(arg2)+1;
    }
    CHECK_EXIT("calloc sms_write->str", sms_write->str, calloc(sms_write->len+1, sizeof(unsigned char)), NULL)
    // fare un print in esadecimale per vedere se ci sono rimasugli di
    strncpy((char *) sms_write->str, api_id, 3);
    strncat((char *)sms_write->str, arg1, strlen(arg1));
    strncat((char *)sms_write->str, ",", 2);
    if (arg2 != NULL) {
        strncat((char *)sms_write->str, arg2, strlen(arg2));
    }
    int notused;
    CHECK_EXIT("write ClConn size", notused, writen(fd_c, &sms_write->len, sizeof(size_t)), -1)
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_write->str, sms_write->len), -1)

    free(sms_write->str);
    free(sms_write);
    printf("\nRichiesta chiusura socket client inviata...\n");
    return 0;
}



void recievedMsg_ServerToClient_Read(char ** pathname, unsigned char** sms_content, size_t *size_buf,int * check, int fd_c){
    // check + size + contenuto
    unsigned char * sms_info;
    recievedMsg_ServerToClient(&sms_info, fd_c);
    msg_t * sms;
    CHECK_EXIT("calloc read", sms, calloc(1, sizeof(msg_t)), NULL)
    char * tmp;
    char * token = strtok_r((char *)sms_info, ",", &tmp);
    if (strncmp(token, "1", 1)==0) { // okay, il contenuto di un file in arrivo dallo storage
        token = strtok_r(NULL, ",", &tmp);
        int size_byte = strtol(token, NULL, 10);
        token = strtok_r(NULL, ",", &tmp);
        *pathname = token;
        CHECK_EXIT("calloc sms", sms->str, calloc(size_byte, sizeof(unsigned char)), NULL)
        sms->len = size_byte;
        int n;
        CHECK_EXIT("read ClConn sms", n, readn(fd_c, sms->str, sms->len), -1)
        *sms_content = sms->str;
        *check = 1;
    } else if (strncmp(token, "0", 1)==0){ // non ci sono piu file da leggere
        *size_buf = -1;
        *sms_content = NULL;
        *check = 0;
        *pathname = NULL;
    } else { // errore, qualcosa e' andato storto
        *size_buf = -1;
        *sms_content = NULL;
        *check = -1;
        *pathname = NULL;
    }
    free(sms);

}


void recievedMsg_ServerToClient(unsigned char ** sms_info, int fd_c) {
    msg_t *sms_read;
    int notused;
    CHECK_EXIT("calloc read", sms_read, calloc(1, sizeof(msg_t)), NULL)
    CHECK_EXIT("read ClConn size", notused, readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
    CHECK_EXIT("calloc read", sms_read->str, calloc(1, sms_read->len), NULL)
    CHECK_EXIT("read ClConn sms", notused, readn(fd_c, sms_read->str, sms_read->len), -1)
    *sms_info = sms_read->str;
    free(sms_read);
}


// -----------------------------  SERVER  -----------------------------

int sendMsg_ServerToClient(int fd_client,unsigned char * arg1, unsigned char * arg2, unsigned char* arg3) {

    return 0;
}
void recievedMsg_ClientToServer(unsigned char ** sms_info, int fd_c) {
    return;
}
/*!
 * Crea e controlla il path assoluto se esiste
 *
 * @param pathname
 * @param abs_path
 * @return
 */
int find_absolute_path(char* pathname, char **abs_path){ // funzione di controllo path assoluto
    char *buf = pathname;
    char * abslotue_path;
    abslotue_path = realpath(buf, NULL);
    if(abslotue_path == NULL || errno==ENOENT){
        printf("cannot find file with name[%s]\n", buf);
        free(abslotue_path);
        return -1;
    } else{
        *abs_path = abslotue_path;
        //printf("path[%s]\n", abslotue_path);
        free(abslotue_path);
        return 0;
    }
}

/*!
 *
 * @param dir
 * @return
 */
int isdot(const char dir[]) {
    int l = (int) strlen(dir);
    if ( (l>0 && dir[l-1] == '.') ) return 1;
    return 0;
}

/*!
 * Check a fondo per trovare n file nella directory dirname
 *
 * @param dirname
 * @param lst_of_files
 * @param nfiles
 * @param ram_sz
 * @param index
 * @return
 */
int recDirectory(char * dirname, char ** lst_of_files, long *nfiles, int *index) {
    DIR *dir= opendir(dirname);
    if(dir == NULL) {
        perror("opendir");
        return 1;
    } else {
        fprintf(stdout, "-----------------------\n");
        fprintf(stdout, "Directory %s:\n",dirname);
        struct dirent* ent; // file, directory etc..
        while ((ent = readdir(dir)) != NULL) { // itera e stampa tutto cio che si trova nella cartella dirname
            int lendir = (int) strlen(dirname) + 1;
            int lenstanga = strlen("/") + 1;
            //int lenname = strlen(ent->d_name) + 1;
            int len = lendir + lenstanga + BUFSIZE;// 1 e' per '\0'
            char next_path[len];
            memset(next_path, '\0', len);
            strncpy(next_path, dirname, strlen(dirname));
            strncat(next_path, "/", 2);
            strncat(next_path, ent->d_name, BUFSIZE);
            struct stat st;
            stat(next_path, &st); // controllo dell'errore, va aggiunto
            if(*nfiles < 1) {
                printf("ricerca finita\n");
                closedir(dir);
                return 0;
            } else {
                if (S_ISDIR(st.st_mode)) {
                    // se est una directory allora entro in profondita'
                    // e' una directory
                    if(!isdot(next_path)) {
                        recDirectory(next_path, lst_of_files, nfiles, index); // con la recursione
                    }
                } else {
                    // e' un file
                    char * dir_abspath;
                    if (find_absolute_path(next_path, &dir_abspath) == -1) {
                        printf("absolute path of dirname isn't correct\n");
                        exit(EXIT_FAILURE);
                    }
                    int i = *index;
                    lst_of_files[i] = dir_abspath;
                    //fprintf(stdout, "%20s: nfile: %d, CONTENUTO : %s\n\n", ent->d_name, *nfiles, lst_of_files[*index]);
                    --*nfiles;
                    ++(*index);
                }
            }
        }
        fprintf(stdout, "-----------------------\n");
        closedir(dir);
        return 0;

    }
}
