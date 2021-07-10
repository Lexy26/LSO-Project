

#ifndef UTIL_CLIENT_H
#define UTIL_CLIENT_H

#include "util.h"

void timer(int time);

void initLstRequest(nb_request ** lst_request);

void createRequest(char *ptarg, char *dirname, char option[2], nb_request ** pRequest, int *index);

void freer(command_t **lst, size_t sz);

int find_absolute_path(char* pathname, char **abs_path);

int openAppendClose(char * pathname, nb_request ** nbRequest, int index);

int isdot(const char dir[]);

int recDirectory(char * dirname, long *nfiles, int index, int *totfile, nb_request** nbRequest);

#endif //UTIL_CLIENT_H
