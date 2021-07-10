/*CLIENT*/
#define _POSIX_C_SOURCE  200112L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>


#include "./includes/util.h"
#include "./includes/util_client.h"
#include "./includes/API.h"

#define test 1


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: ./client option [parameters]\n"
               "   or: ./client -h (help)\n");
        exit(EXIT_FAILURE);
    }
    // initialise list for client requests
    nb_request * nbRequest;
    initLstRequest(&nbRequest);
    int index = 0;
    int opt;
    opterr = 0;
    extern char *optarg;
    extern int optopt;
    // this loop take the client requests and put them in the array nbRequest
    while ((opt = getopt(argc, argv, "p c:u:h l:t:d:R:r:f:w:W:")) != -1) {
        switch (opt) {
            case 'h':
                nbRequest->char_h = 1;
                break;
            case 'f':
                nbRequest->char_f = optarg;
                break;
            case 'p':
                nbRequest->char_p = 1;
                break;
            case 'w':
                createRequest(optarg, NULL, "w", &nbRequest, &index);
                break;
            case 'W':
                createRequest(optarg, NULL, "W", &nbRequest, &index);
                break;
            case 'r':
                createRequest(optarg, NULL, "r", &nbRequest, &index);
                break;
            case 'R':
                createRequest(optarg, NULL, "R", &nbRequest, &index); // argv[optind]
                break;
            case 'd': {
                char *dirname = optarg;
                if ((opt = getopt(argc, argv, "r:R:")) != -1) {
                    if (opt == 'r') {
                        createRequest(optarg, dirname, "r", &nbRequest,&index);
                        break;
                    } else if (opt == 'R') {
                        createRequest(optarg, dirname, "R", &nbRequest,&index);
                        break;
                    } else { // error
                        printf("Usage -d option: -d dirname -r file1[,file2]\n\t     or: -d dirname -R n=0\n");
                        return -1;
                    }
                } else { // error
                    printf("Usage -d option: -d dirname -r file1[,file2]\n\t     or: -d dirname -R n=0\n");
                    return -1;
                }
            }
            case 't':
                nbRequest->char_t = (int) strtol(optarg, NULL, 10);
                break;
            case 'D':
                printf("-D under construction\n");
                break;
            case 'l':
                printf("-l under construction\n");
                break;
            case 'u':
                printf("-u under construction\n");
                break;
            case 'c':
                printf("-c under construction\n");
                break;
            case ':':
                printf("-%c argomento mancante\n", opt);
                printf("usage: ./client option [parameters]\n"
                       "   or: ./client -h (help)\n");
                exit(EXIT_FAILURE);
            case '?':
                printf("-%c opzione non corretta\n", optopt);
                printf("usage: ./client option [parameters]\n"
                       "   or: ./client -h (help)\n");
                exit(EXIT_FAILURE);
            default: ;
        }
    }
    // the socket name must be given from cmd line
    if (nbRequest->char_f == NULL) {
        fprintf(stderr, "Socket error\n");
        return -1;
    } else {
        // connection with server
        char * abs_path_socket;
        if (find_absolute_path(nbRequest->char_f, &abs_path_socket) == -1) {
            fprintf(stderr, "socket name ");
            exit(EXIT_FAILURE);
        }
        if (simple_opneConnection(abs_path_socket, 1, 10)== -1){
            fprintf(stderr, "Open Connection error\n");
            exit(EXIT_FAILURE);
        }
    }
    int cond = 1;
    if (nbRequest->char_h) { // help
        PRINT_H
        cond = 0;
    }
    // while loop to iterate requests to be send to the server
    index = 0;
    while (cond && index < nbRequest->tot_request) {
        char options[2];
        strncpy(options, nbRequest->lst_char_abc[index]->option, 2);
        if (strcmp(options, "w") == 0) { // -w dirname [,n=0]
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",=", &tmp); // token = dirname | tmp = n=0
            long nfile;
            char * dirname = token;
            struct stat st;
            int correct = 1;
            if (stat(dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
                fprintf(stderr, "Directory doesn't exist\n");
                correct = -1;
            } else {
                token = strtok_r(NULL, ",=", &tmp);
                if (token != NULL) { // check if n was given
                    token = strtok_r(NULL, "=,", &tmp); // token1 = n | tmp1 = 0
                    if(token != NULL) { // check that the request is well written
                        nfile = strtol(tmp, NULL, 10);
                        if(nfile<0) nfile = 0;
                    } else {
                        printf("Usage -w option : -w dirname[,n=0]\n");
                        correct = -1;
                    }
                    nfile = strtol(token, NULL, 10);
                    if(nfile<0) { nfile = 0; }
                } else { nfile = 0; }
                int totfile = 0;
                if(nfile > 0) {
                    totfile = -1;
                }
                if(correct == 1) {
                    if (nbRequest->char_p == 1) {
                        fprintf(stderr, " REQUEST ==== Write [%ld] file from directory [%s] ====\n", nfile, dirname);
                    }
                    // send request to write n file
                    recDirectory(dirname, &nfile, index, &totfile, &nbRequest);
                }
            }
            ++index;
        }
        else if (strcmp(options, "W") == 0) { // -W file1[,file2]
            if (nbRequest->char_p == 1) {
                fprintf(stderr, " REQUEST ==== Write the following file : %s ====\n", nbRequest->lst_char_abc[index]->param);
            }
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
            char *abs_path_token;
            // loop to write files into the server storage
            while (token) {
                abs_path_token = NULL;
                if(find_absolute_path(token, &abs_path_token) == -1) {
                    fprintf(stderr, "Path of file [%s] doesn't exist\n", token);
                } else {
                    if (openAppendClose(abs_path_token, &nbRequest, index) == -1) {
                        fprintf(stderr, "Can't write file : %s\n", token);
                    }
                }
                token = strtok_r(NULL, ",", &tmp);
                timer(nbRequest->char_t);
            }
            ++index;
        }
        else if (strcmp(options, "r") == 0) { // -r file1[,file2] [-d dirname]
            if (nbRequest->char_p == 1) {
                fprintf(stderr, " REQUEST ==== Read the following file : %s ====\n", nbRequest->lst_char_abc[index]->param);
            }
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
            void *buffer;
            size_t size_buffer;
            char *abs_path_token;
            // loop to read files from the server storage
            while (token != NULL) {
                abs_path_token = NULL;
                if (find_absolute_path((char*)token, &abs_path_token) == -1) {
                    fprintf(stderr, "Path of file [%s] doesn't exist\n", token);
                } else {
                    if (openFile(abs_path_token, O_OPEN) == 0) {
                        if (readFile(abs_path_token, &buffer, &size_buffer) == -1) {
                            fprintf(stderr, "Can't read file : %s\n", token);
                        } else {
                            struct stat st;
                            char * dirname = nbRequest->lst_char_abc[index]->dirname;
                            // sposta questo controllo fuori dal loop cosi nmon lo stampo un sacco di
                            // volte oppure metti una variabile di controllo che lo faccia stampare una volta sola
                            if (dirname != NULL) {
                                if (stat(dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
                                    fprintf(stderr, "Directory [%s] doesn't exist.\nCan't save files read.\n", dirname);
                                } else {
                                    char * dirname_abs_path;
                                    if (find_absolute_path((char*)dirname, &dirname_abs_path) == -1) {
                                        fprintf(stderr, "Path of file [%s] doesn't exist\n", token);
                                    }
                                    // create new path to insert file read in given directory
                                    char * base_path = basename(token);
                                    char * filename;
                                    CHECK_EXIT_VAR("malloc filename", filename, malloc((strlen(dirname_abs_path) +2+ strlen(base_path))* sizeof(unsigned char)), NULL)
                                    memset(filename, 0,(strlen(dirname_abs_path) +2+ strlen(base_path)));
                                    strncpy(filename, dirname_abs_path, strlen(dirname_abs_path));
                                    strncat(filename, "/", strlen("/")+1);
                                    strncat(filename, base_path, strlen(base_path)+1);
                                    FILE *f;
                                    CHECK_EXIT_VAR("fopen f", f, fopen(filename, "wb"), NULL)
                                    CHECK_EQ_EXIT("fwrite f", fwrite(buffer, size_buffer, 1, f), -1)
                                    fclose(f);
                                }
                            }
                        }
                        timer(nbRequest->char_t);
                        if (closeFile(abs_path_token) == -1) {
                            fprintf(stderr, "Close File error\n");
                            closeConnection(nbRequest->char_f);
                        } else {
                            printf( "Read file [ %s ] with size [%zu] done\n", token, size_buffer);
                        }
                    } else {
                        fprintf(stderr, "File [%s] doesn't exist\n", token);
                    }
                }
                token = strtok_r(NULL, ",", &tmp);
                timer(nbRequest->char_t);
            }
            //free(nbRequest->lst_char_abc[index]);
            ++index;
        } else if (strcmp(options, "R") == 0) { // -R [n=0] [-d dirname]
            int nb_files;
            if (nbRequest->lst_char_abc[index]->param != NULL) { // se n e' stato adto, allora tokenizzo
                //unsigned long len = strlen(nbRequest->lst_char_abc[index]->param)-1;
                char *tmp;
                char * token = strtok_r(nbRequest->lst_char_abc[index]->param, "=",&tmp);
                token = strtok_r(NULL, "=", &tmp);
                nb_files =(int) strtol(token, NULL, 10);
                if(nb_files < 0) nb_files = 0;
                else nb_files = nb_files;
            } else {
                nb_files = 0;
            }
            printf(" REQUEST ==== Read [%d] file ==== \n", nb_files);
            if(readNFiles(nb_files, nbRequest->lst_char_abc[index]->dirname) == -1) {
                printf("Error in read %d file\n", nb_files);
            }
            //free(nbRequest->lst_char_abc[index]);
            timer(nbRequest->char_t);// pausa fra una richiesta e l'altra
            ++index;
        }
        fprintf(stderr, "\n\n");
        //printf("                 INDEX dopo : %d---------------------\ncommand : %s\n", index, nbRequest->lst_char_abc[index]->param);
    }

    // CONCLUSIONE
    fprintf(stderr, "CHIUSURA TOTALE\n");
    if (closeConnection(nbRequest->char_f) == -1) { // chiedo al server di chiudere la connessione con questo client
        fprintf(stderr, "Close connection error\n");
    } else {
        fprintf(stderr, "Close connection no problem\n");
    }
    //freer(&lst_char_abc, LST_SZ);
    fprintf(stderr, "\n");
    // il client chiude il suo canale lo fa direttamente deentro closeconnection
    return EXIT_SUCCESS;

}
