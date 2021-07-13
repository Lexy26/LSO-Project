#define _POSIX_C_SOURCE  200112L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <getopt.h>

#include "configuration.h"

#define BUF_SIZE 256

char *strndup(const char *s, size_t n);


/*!
 * configuration : This function reads the configuration file and extracts information
 * @param cfg : is where i put the output
 */
void configuration(int argc,char *argv[],config_t **cfg){
    if (argc != 3) {
        fprintf(stderr,"Usage: ./server -F <fileConfig.txt>\n");
        exit(EXIT_FAILURE);
    }
    FILE * config;
    config = NULL;
    int opt;
    opterr = 0;
    (*cfg)->LOGFILE = NULL;
    (*cfg)->SOCKNAME = NULL;
    (*cfg)->N_THREAD = 0;
    (*cfg)->N_FILE = 0;
    (*cfg)->MEM_SIZE = 0;
    if ((opt = getopt(argc, argv, "F:")) != -1) {
        if (opt == 'F') {
            if ((config = fopen(optarg, "r")) == NULL) {
                perror("config fopen");
                exit(errno);
            }
        } else {
            fprintf(stderr,"Invalid option\n");
            fprintf(stderr,"Usage: ./server -F <fileConfig.txt>\n");
            exit(EXIT_FAILURE);
        }
    }
    char buffer[BUF_SIZE];
    char *tmp;
    char *token;
    //           line_out   size    file
    while (fgets(buffer, BUF_SIZE, config) != NULL) {
        char *correct = NULL;
        token = strtok_r(buffer, " ", &tmp);
        if (strncmp(token, "N_THREAD", strlen("N_THREAD")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);
            if (correct != NULL && *correct == (char) 0) {// error check
                if(number < 1) {
                    fprintf(stderr,"N_THREAD convertion error\n");
                    fprintf(stderr,"server starting with N_THREAD=1 default variable\n");
                    (*cfg)->N_THREAD = 1;
                } else (*cfg)->N_THREAD = (int) number;
            } else {
                fprintf(stderr,"N_THREAD convertion error\n");
                fprintf(stderr,"server starting with N_THREAD=1 default variable\n");
                (*cfg)->N_THREAD = 1;
            }
        } else if (strncmp(token, "N_FILE", strlen("N_FILE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);
            if (correct != NULL && *correct == (char) 0) {
                if(number < 1) {
                    fprintf(stderr,"N_FILE convertion error\n");
                    fprintf(stderr,"server starting with N_FILE=50 default variable\n");
                    (*cfg)->N_FILE = 50;
                } else (*cfg)->N_FILE = (int) number;
            } else {
                fprintf(stderr,"N_FILE convertion error\n");
                fprintf(stderr,"server starting with N_FILE=50 default variable\n");
                (*cfg)->N_FILE = 50;
            }
        } else if (strncmp(token, "MEM_SIZE", strlen("MEM_SIZE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);
            if (correct != NULL && *correct == (char) 0) {
                if(number < 1) {
                    fprintf(stderr,"MEM_SIZE convertion error\n");
                    fprintf(stderr,"server starting with MEM_SIZE=128000 default variable\n");
                    (*cfg)->MEM_SIZE = 128000;
                } else (*cfg)->MEM_SIZE = (int) number;
            } else {
                fprintf(stderr,"MEM_SIZE convertion error\n");
                fprintf(stderr,"server starting with MEM_SIZE=128000 default variable\n");
                (*cfg)->MEM_SIZE = 128000;
            }


        } else if (strncmp(token, "SOCKNAME", strlen("SOCKNAME")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            if(token == NULL || strncmp(&token[0], "#", 1)==0){
                fprintf(stderr,"SOCKNAME convertion error\n");
                fprintf(stderr,"server starting with SOCKNAME=./cs_sock default variable\n");
                (*cfg)->SOCKNAME = strndup( "./cs_sock", strlen("./cs_sock"));
            } else {
                (*cfg)->SOCKNAME = strndup(token, strlen(token));
            }


        } else if (strncmp(token, "LOGFILE", strlen("LOGFILE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            if(token == NULL || strncmp(&token[0], "#", 1)==0){
                fprintf(stderr,"LOGFILE convertion error\n");
                fprintf(stderr,"server starting with LOGFILE=logfile.txt default variable\n");
                (*cfg)->LOGFILE = strndup( "logfile.txt", strlen("logfile.txt"));
            } else {
                (*cfg)->LOGFILE = strndup( token, strlen(token));
            }
        }
    }
    if((*cfg)->N_FILE == 0) {
        fprintf(stderr,"N_FILE convertion error\n");
        fprintf(stderr,"server starting with N_FILE=50 default variable\n");
        (*cfg)->N_FILE = 50;
    }
    if ((*cfg)->N_THREAD == 0) {
        fprintf(stderr,"N_THREAD convertion error\n");
        fprintf(stderr,"server starting with N_THREAD=1 default variable\n");
        (*cfg)->N_THREAD = 1;
    }
    if ((*cfg)->MEM_SIZE == 0) {
        fprintf(stderr,"MEM_SIZE convertion error\n");
        fprintf(stderr,"server starting with MEM_SIZE=128000 default variable\n");
        (*cfg)->MEM_SIZE = 128000;
    }
    if ((*cfg)->LOGFILE == NULL) {
        fprintf(stderr,"LOGFILE convertion error\n");
        fprintf(stderr,"server starting with LOGFILE=logfile.txt default variable\n");
        (*cfg)->LOGFILE = strndup( "logfile.txt", strlen("logfile.txt"));
    }
    if ((*cfg)->SOCKNAME == NULL) {
        fprintf(stderr,"SOCKNAME convertion error\n");
        fprintf(stderr,"server starting with SOCKNAME=./cs_sock default variable\n");
        (*cfg)->SOCKNAME = strndup( "./cs_sock", strlen("./cs_sock"));
    }
    fclose(config);
}
