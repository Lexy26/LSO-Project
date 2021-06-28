

#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "util_client.h"
#include "API.h"

#define T_REQUEST 50// limite massimo di richieste possibili da parte di questo client
#define BUFSIZE 256

char *realpath(const char *path, char *resolved_path);


void timer(int time) {
    if (time != 0) { // prima che faccia la prossima richiesta aspetta char_t di tempo in
        sleep(time/1000); // divido per 1000 per avere i millisecondi
    }
}

void initLstRequest(nb_request ** lst_request) {
    CHECK_EXIT("calloc lst", *lst_request, calloc(1, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    CHECK_EXIT("calloc lst", (*lst_request)->lst_char_abc, calloc(T_REQUEST, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    memset((*lst_request)->lst_char_abc, 0, T_REQUEST* sizeof(nb_request));
    (*lst_request)->char_f = NULL;
    (*lst_request)->char_h = 0;
    (*lst_request)->char_p = 0;
    (*lst_request)->char_t = 0;
}

void createRequest(char *ptarg, char *dirname, char option[2], nb_request ** pRequest, int *index) {
    command_t * char_abc;
    CHECK_EXIT("calloc", char_abc, calloc(1, sizeof(command_t)), NULL)
            (char_abc)->param = ptarg;
    (char_abc)->dirname = dirname;
    strncpy((char_abc)->option, option, 2);
    (*pRequest)->lst_char_abc[*index] = char_abc;
    ++(*pRequest)->tot_request;
    ++*index;
}

// *** lst : xke' 1) * equal to command_t in array; 2) * per indicare che e' un array, 3) * perche' non voglio che le modifiche rimangano in locale
void freer(command_t **lst, size_t sz) {
    for (int i = 0; i < sz; ++i) {
        //free(lst[i]->param);
        free(lst[i]);
    }
    free(lst);
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
        //free(abslotue_path);
        return -1;
    } else{
        *abs_path = abslotue_path;
        //free(abslotue_path);
        return 0;
    }
}

void openAppendClose(char * pathname, nb_request ** nbRequest, int index) {
    if (openFile(pathname, O_CREATE) != 0) { // se file non sessiste e va creato
        if (openFile(pathname, O_OPEN)==-1) {
            perror("open append close : Open");
            exit(errno);
        }// se file gia esistente
    }
    timer((*nbRequest)->char_t); // poiche openFile est una richiesta
    FILE * f = fopen(pathname, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * buf = calloc(fsize + 1, sizeof(char));
    fread(buf, fsize, 1, f);
    fclose(f);
    buf[fsize] = 0;
    //printf("file da inserire : %s\n", buf);
    if(appendToFile(pathname, buf, fsize, (*nbRequest)->lst_char_abc[index]->dirname)==-1) {
        perror("open append close : Append");
        exit(errno);
    }
    free(buf);
    printf("!!!!!   Elemento inserito   !!!!!!!\n");
    timer((*nbRequest)->char_t);
    if(closeFile(pathname)==-1) {
        perror("open append close : Close");
        exit(errno);
    }
    printf("!!!!!   Elemento chiuso   !!!!!!!\n");
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
int recDirectory(char * dirname, char ** lst_of_files, long *nfiles, int index, int *totfile, nb_request **nbRequest) {
    DIR *dir= opendir(dirname);
    if(dir == NULL) {
        perror("opendir");
        return 1;
    } else {
        fprintf(stdout, "------------------Directory %s:-----------------\n", dirname);
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
            if(*nfiles < 1 && *totfile < 0) {
                fprintf(stdout, "-----------------------------------------------\n");
                closedir(dir);
                return 0;
            } else {
                if (S_ISDIR(st.st_mode)) {
                    // se est una directory allora entro in profondita'
                    // e' una directory
                    if(!isdot(next_path)) {
                        recDirectory(next_path, lst_of_files, nfiles, index, totfile, nbRequest); // con la recursione
                    }
                } else {
                    // e' un file
                    char * dir_abspath;
                    if (find_absolute_path(next_path, &dir_abspath) == -1) {
                        printf("absolute path of dirname isn't correct\n");
                        exit(EXIT_FAILURE);
                    }
                    if (*totfile < 0) { // nel caso sapessi il numero totale dei file da estrapolare totfile = -1
                        printf("file : %s\n", dir_abspath);

                        openAppendClose(dir_abspath, nbRequest, index);
                        --*nfiles;
                    } else {
                        printf("file : %s\n", dir_abspath);
                        openAppendClose(dir_abspath, nbRequest, index);
                    }
                }
            }
        }
        fprintf(stdout, "-----------------------\n");
        closedir(dir);
        return 0;

    }
}
