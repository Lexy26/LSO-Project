

#ifndef UTIL_CLIENT_H
#define UTIL_CLIENT_H

#include "util.h"
// structure for command information
typedef struct {
    char option[2]; // operation type
    char *param; // optarg, what comes after the opt
    char * dirname; // useful only with -r and -R
} command_t;

// structure to list requests
typedef struct {
    int tot_request; // total number of requests (without -t, -h, -p, -f)
    int char_t; // time command
    int char_h; // help command
    int char_p; // print command
    char* char_f; // socket name for connection
    command_t ** lst_char_abc; // array of all requests
}nb_request;

void timer(int time);

void initLstRequest(nb_request ** lst_request);

void createRequest(char *ptarg, char *dirname, char option[2], nb_request ** pRequest, int *index);

void freer(command_t **lst, size_t sz);

int find_absolute_path(char* pathname, char **abs_path);

int openAppendClose(char * pathname, nb_request ** nbRequest, int index);

int isdot(const char dir[]);

int recDirectory(char * dirname, long *nfiles, int index, int *totfile, nb_request** nbRequest);

#endif //UTIL_CLIENT_H
