
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>



#include "util_client.h"
#include "API.h"


char *realpath(const char *path, char *resolved_path);





/*!
 * initLstRequest : initialise list of client requests
 * @param lst_request and return: list of request initialised
 */
void initLstRequest(nb_request ** lst_request) {
    CHECK_EXIT_VAR("calloc initLstRequest lst_request", *lst_request, calloc(1, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    CHECK_EXIT_VAR("calloc initLstRequest list", (*lst_request)->lst_char_abc, calloc(1,sizeof(command_t)),NULL)// spazio suff. per freeare char_abc + msg
    memset((*lst_request)->lst_char_abc, 0, sizeof(command_t));
    (*lst_request)->char_f = NULL;
    (*lst_request)->char_h = 0;
    (*lst_request)->char_p = 0;
    (*lst_request)->char_t = 0;
}

/*!
 * createRequest : create structure of request
 * @param ptarg : argument of option
 * @param dirname : for -r and -R option
 * @param option : determine type of client request
 * @param pRequest : to insert new request in list
 * @param index : index of list
 */
void createRequest(char *ptarg, char *dirname, char option[2], nb_request ** pRequest, int *index) {
    command_t * char_abc;
    CHECK_EXIT_VAR("calloc createRequest", char_abc, calloc(1, sizeof(command_t)), NULL)
    (char_abc)->param = ptarg;
    (char_abc)->dirname = dirname;
    strncpy((char_abc)->option, option, 2);
    int i = *index;
    CHECK_EXIT_VAR("realloc", (*pRequest)->lst_char_abc, realloc((*pRequest)->lst_char_abc, (i+1)* sizeof((*pRequest)->lst_char_abc)), NULL)

    (*pRequest)->lst_char_abc[i] = &(*char_abc);
    ++(*pRequest)->tot_request;
    *index += 1;
}

/*!
 * freer      : free the whole list
 * @param lst : list of requests
 * @param sz  : size of list
 */
void freer(command_t **lst, size_t sz) {
    for (int i = 0; i < sz; ++i) {
        free(lst[i]);
    }
    free(lst);
}


/*!
 * find_absolute_path : Create and check the absolute path
 * @param pathname    : name of file that i want the path
 * @param abs_path    : here is the absolute path
 * @return            : is in abs_path
 */
int find_absolute_path(char* pathname, char **abs_path){
    char *buf = pathname;
    char * abslotue_path;
    errno = 0;
    abslotue_path = realpath(buf, NULL);
    if(abslotue_path == NULL || errno==ENOENT){
        return -1;
    } else{
        *abs_path = abslotue_path;
        return 0;
    }
}
/*!
 * openAppendClose  : this function ask to open, append content and close file to the server
 * @param pathname  : pathname of file, that i want read
 * @param nbRequest : list of client requests
 * @param index     : index of list
 * @return          : -1 error, 0 okay
 */
int openAppendClose(char * pathname, nb_request ** nbRequest, int index) {
    // request to create the file
    if (openFile(pathname, O_CREATE) != 0) {
        // request to open the file
        if (openFile(pathname, O_OPEN)==-1) {
            fprintf(stderr, "Open File [%s] error\n", pathname);
            return -1;
        }
    }
    timer((*nbRequest)->char_t);
    // read file content
    FILE * f;
    CHECK_EXIT_VAR("fopen openAppendClose", f, fopen(pathname, "rb"), NULL)
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char * buf;
    CHECK_EXIT_VAR("calloc buf", buf, calloc(fsize + 1, sizeof(char)), NULL)
    fread(buf, fsize, 1, f);
    fclose(f);
    buf[fsize] = 0;
    if(appendToFile(pathname, buf, fsize, (*nbRequest)->lst_char_abc[index]->dirname)==-1) {
        fprintf(stderr, "Append File [%s] error\n", pathname);
        return -1;
    }
    free(buf);
    timer((*nbRequest)->char_t);
    if(closeFile(pathname)==-1) {
        fprintf(stderr, "Close File [%s] error\n", pathname);
        return -1;
    }
    if ((*nbRequest)->char_p == 1) {
        fprintf(stderr, "File : %s\nSize : %ld\n", pathname, fsize);
    }
    return 0;
}

/*!
 * isdot      : check if current dir is a point
 * @param dir : current directory
 * @return : 0 okay, 1 not okay
 */
int isdot(const char dir[]) {
    int l = (int) strlen(dir);
    if ( (l>0 && dir[l-1] == '.') ) return 1;
    return 0;
}


/*!
 * recDirectory     : recursive search of nfile or all
 * @param dirname   : name of directory
 * @param nfiles    : number of file to ask to insert in storage to server
 * @param index     : index of requests list
 * @param totfile   : in case you know the total number of files to extract totfile = -1
 * @param nbRequest : list of requests
 * @return
 */
int recDirectory(char * dirname, long *nfiles, int index, int *totfile, nb_request **nbRequest) {
    DIR *dir;
    CHECK_EXIT_VAR("opendir", dir, opendir(dirname), NULL)
    struct dirent* ent; // file, directory etc..
    while ((ent = readdir(dir)) != NULL) {
        int lendir = (int) strlen(dirname) + 1;
        int lenstanga = strlen("/") + 1;
        int len = lendir + lenstanga + BUFSIZE;
        char next_path[len];
        memset(next_path, '\0', len);
        strncpy(next_path, dirname, strlen(dirname));
        strncat(next_path, "/", 2);
        strncat(next_path, ent->d_name, BUFSIZE);
        struct stat st;
        CHECK_EQ_EXIT("stat", stat(next_path, &st), -1)
        if(*nfiles < 1 && *totfile < 0) {
            closedir(dir);
            return 0;
        } else {
            if (S_ISDIR(st.st_mode)) {
                if(!isdot(next_path)) {
                    recDirectory(next_path, nfiles, index, totfile, nbRequest);
                }
            } else {
                // is a file
                char * dir_abspath;
                if (find_absolute_path(next_path, &dir_abspath) == -1) {
                    fprintf(stderr, "Path of directory [%s] doesn't exist\n", next_path);
                    exit(EXIT_FAILURE);
                }
                if (*totfile < 0) {
                    openAppendClose(dir_abspath, nbRequest, index);
                    --*nfiles;
                } else {
                    openAppendClose(dir_abspath, nbRequest, index);
                }
                free(dir_abspath);
            }
        }
    }
    closedir(dir);
    return 0;
}
