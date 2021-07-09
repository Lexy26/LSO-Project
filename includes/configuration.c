#define _POSIX_C_SOURCE  200112L

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <getopt.h>

#include "configuration.h"


#define BUF_SIZE 256

char *realpath(const char *path, char *resolved_path);

void configuration(int argc,char *argv[],config_t **cfg){
    if (argc != 3) {// ~$ ./server -F config1.txt (null)
        printf("Use: ./server -F <fileConfig.txt>\n");
        exit(EXIT_FAILURE);
    }
    FILE * config;
    config = NULL;
    int opt;
    opterr = 0; // disattivo l'output della riga './out: invalid option -- 'g''
    // cosi il file scritto di seguito a '-F' so che sara' un file config
    if ((opt = getopt(argc, argv, "F:")) != -1) {
        if (opt == 'F') {
            if ((config = fopen(optarg, "r")) == NULL) {
                perror("config fopen");
                exit(errno);
            }
        } else {
            printf("Invalid option\n");
            printf("Use: ./server -F <fileConfig.txt>\n");
            exit(EXIT_FAILURE);
        }
    }
    // qui ricopio il contenuto di config file nelle varie variabili della struct config_t
    char buffer[BUF_SIZE]; // qui e' dove mettero' le righe del file config1.txt
    char *tmp;// mi serve per far strtok_r rientrante
    char *token;
    // per leggere riga per riga quello che sta dentro al file
    //           line_out   size    file
    while (fgets(buffer, BUF_SIZE, config) != NULL) {
        char *correct = NULL;
        token = strtok_r(buffer, " ", &tmp);
        if (strncmp(token, "N_THREAD", strlen("N_THREAD")) ==
            0) {// i due casi di seguito sono una ripetizione di questo, vedi di fare una funzione in futuro
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);// trasformo il num stringa in int.
            if (correct != NULL && *correct == (char) 0) {// controllo che non ci siano errori
                if(number < 1) {
                    printf("N_THREAD convertion error\n");
                    printf("server starting with N_THREAD=5 default variable");
                    (*cfg)->N_THREAD = 5;
                } else (*cfg)->N_THREAD = (int) number;
                //printf("N_THREAD cfg %d\n", cfg->N_THREAD);
            } else {
                printf("N_THREAD convertion error\n");
                printf("server starting with N_THREAD=5 default variable");
                (*cfg)->N_THREAD = 5;
            }
        } else if (strncmp(token, "N_FILE", strlen("N_FILE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);// trasformo il num sringa in int.
            if (correct != NULL && *correct == (char) 0) {
                if(number < 1) {
                    printf("N_THREAD convertion error\n");
                    printf("server starting with N_FILE=50 default variable");
                    (*cfg)->N_FILE = 50;
                } else (*cfg)->N_FILE = (int) number;
                //("N_FILE cfg %d\n", cfg->N_FILE);
            } else {
                perror("N_FILE convertion error");
                printf("server starting with N_FILE=50 default variable");
                (*cfg)->N_FILE = 50;
            }
        } else if (strncmp(token, "MEM_SIZE", strlen("MEM_SIZE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);// trasformo il num sringa in int.
            if (correct != NULL && *correct == (char) 0) {
                if(number < 512) {
                    perror("MEM_SIZE convertion error");
                    printf("server starting with MEM_SIZE=128000 default variable");
                    (*cfg)->MEM_SIZE = 128000;
                } else (*cfg)->MEM_SIZE = (int) number;
                //printf("MEM_SIZE cfg %d\n", cfg->MEM_SIZE);
            } else {
                perror("MEM_SIZE convertion error");
                printf("server starting with MEM_SIZE=128000 default variable");
                (*cfg)->MEM_SIZE = 128000;
            }
        } else if (strncmp(token, "SOCKNAME", strlen("SOCKNAME")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            if(token == NULL || strncmp(&token[0], "#", 2)==0){
                perror("SOCKNAME convertion error");
                printf("server starting with SOCKNAME=./cs_sock default variable");
                (*cfg)->SOCKNAME = "./cs_sock";
            }
            (*cfg)->SOCKNAME = malloc(strlen(token)+1);
            memset((*cfg)->SOCKNAME, '\0', strlen(token)+1);
            strncpy((*cfg)->SOCKNAME, token, strlen(token)+1);
            printf(" SOCKET : %s\n", (*cfg)->SOCKNAME);
        }
        else {// se per caso il formato del file config non e' corretto allora errore
            printf("format config file error\n");
            fprintf(stderr, "Value of errno: %d\n", errno);
        }
    }
    fclose(config);
}
