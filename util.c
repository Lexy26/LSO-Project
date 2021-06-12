
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
//#include <limits.h>

#include "util.h"
#include "conn.h"



char *realpath(const char *path, char *resolved_path);


/*!
 * Send message to the server
 *
 * @param sockname_or_flag :
 * @param path_files       :
 * @param fd_c             :
 * @param api_id           :
 *
 * @return
 */
int sendMessage_for_server(const char *sockname_or_flag, char *path_files, int fd_c, char api_id[3]) {
    msg_t *sms_write;
    CHECK_EXIT("calloc sms_write", sms_write, calloc(1, sizeof(msg_t)), NULL)
    sms_write->len = 3;// 3 = per il id della funzione dell'api
    if (sockname_or_flag != NULL) sms_write->len += strlen(sockname_or_flag) + 1; // + 1 per lo spazio da mettere dopo
    if (path_files != NULL) sms_write->len += strlen(path_files) + 1; // per '\0' alla fine
    CHECK_EXIT("calloc sms_write->str", sms_write->str, calloc(sms_write->len, sizeof(char)), NULL)
    // preparo il messaggio con le info da inviare al server
    memset(sms_write->str, '\0', sms_write->len);
    strncpy(sms_write->str, api_id, 3);// api_id ex : "2 "
    if (sockname_or_flag != NULL) {
        strncat(sms_write->str, sockname_or_flag, strlen(sockname_or_flag));
        strncat(sms_write->str, " ", 2);
    } // + 1 per lo spazio da mettere dopo
    if (path_files != NULL) strncat(sms_write->str, path_files, strlen(path_files));

    int notused;
    CHECK_EXIT("write ClConn size", notused, writen(fd_c, &sms_write->len, sizeof(size_t)), -1)
    CHECK_EXIT("write ClConn sms", notused, writen(fd_c, sms_write->str, sms_write->len), -1)
    free(sms_write->str);
    free(sms_write);
    printf("Richiesta chiusura socket client inviata...\n");
    return 0;
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
 * check a fondo per trovare n file nella directory dirname
 *
 * @param dirname
 * @param lst_of_files
 * @param nfiles
 * @param ram_sz
 * @param index
 * @return
 */
int recDirectory(char * dirname, char ** lst_of_files, long *nfiles, long *ram_sz, int *index) {
    DIR *dir= opendir(dirname);
    if(dir == NULL) {
        perror("opendir");
        return 1;
    } else {
        fprintf(stdout, "-----------------------\n");
        fprintf(stdout, "Directory %s:\n",dirname);
        struct dirent* ent; // file, directory etc..
        while ((ent = readdir(dir)) != NULL) { // itera e stampa tutto cio che si trova nella cartella dirname, superficialmente
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
            stat(next_path, &st); // controllo dell'errore
            if(*nfiles < 1 || *ram_sz < 1) {
                printf("ricerca finita\n");
                closedir(dir);
                return 0;
            } else {
                if (S_ISDIR(st.st_mode)) {
                    // se est una directory allora entro in profondita'
                    // e' una directory
                    if(!isdot(next_path)) {
                        recDirectory(next_path, lst_of_files, nfiles, ram_sz, index); // con la recursione
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
                    *ram_sz -= st.st_size;
                    ++(*index);
                }
            }
        }
        fprintf(stdout, "-----------------------\n");
        closedir(dir);
        return 0;

    }
}