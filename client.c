/*CLIENT*/
#define _POSIX_C_SOURCE  200112L

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
//#include <sys/types.h>
//#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>


#include "util.h"
#include "API.h"

#define T_REQUEST 50// limite massimo di richieste possibili da parte di questo client

void timer(int time) {
    if (time != 0) { // prima che faccia la prossima richiesta aspetta char_t di tempo in
        sleep(time/1000); // divido per 1000 per avere i millisecondi
    }
}

void initLstRequest(nb_request ** lst_request) {
    CHECK_EXIT("calloc lst", *lst_request, calloc(1, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    //memset(*lst_request, 0, sizeof(nb_request));
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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: ./client option [parameters]\n"
               "   or: ./client -h (help)\n");
        exit(EXIT_FAILURE);
    }


    // Initialize char variable of each option
    nb_request * nbRequest;
    initLstRequest(&nbRequest);
    int index = 0;
    int opt;
    extern char *optarg;
    extern int optopt;
    // ATTENZIONE ogni char puo avere piu richieste, generalizzare la ricezione delle informazioni per ogni opzione

    while ((opt = getopt(argc, argv, "p c:u:h l:t:d:R r:f:w:W")) != -1) {
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
                createRequest(optarg, NULL, "R", &nbRequest, &index);
                break;
            case 'd': {
                // imettere file letti nel storage
                // se l'opzione successiva non c'e' allora errore EXIT_FAILURE
                char *dirname = optarg;
                if ((opt = getopt(argc, argv, "p c:u:h l:t:d:R r:f:w:W")) != -1) {
                    if (opt == 'r') {
                        createRequest(optarg, dirname, "r", &nbRequest,&index);
                    } else if (opt == 'R') {
                        createRequest(optarg, dirname, "R", &nbRequest,&index);
                    } else { // ERRORE
                        printf("Usage -d option: -d dirname -r file1[,file2]\n\t     or: -d dirname -R [n=0]\n");
                        if (closeConnection(SOCKNAME) == -1) {
                            fprintf(stderr, "Close connection error\n");
                        } else {
                            fprintf(stderr, "Close connection no problem\n");
                        }
                        exit(EXIT_FAILURE);
                    }
                } else { // ERRORE
                    printf("Usage -d option: -d dirname -r file1[,file2]\n\t     or: -d dirname -R [n=0]\n");
                    if (closeConnection(SOCKNAME) == -1) {
                        fprintf(stderr, "Close connection error\n");
                    } else {
                        fprintf(stderr, "Close connection no problem\n");
                    }
                    exit(EXIT_FAILURE);
                }
            }
            case 't':
                nbRequest->char_t = (int) strtol(optarg, NULL, 100);
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
            default:
                printf("default\n");
        }
        //++nbRequest->tot_request;
    }

    if (nbRequest->char_f == NULL) { // usato per test base della connessione
        nbRequest->char_f = SOCKNAME;
        if (simple_opneConnection(SOCKNAME, 1, 10)== -1){
            fprintf(stderr, "errore while open connection to server\n");
            exit(EXIT_FAILURE);
        }
        closeConnection(nbRequest->char_f);
        exit(EXIT_FAILURE);
    } else if (nbRequest->char_f == 0) { // sockanem
        printf("char_f connected with param : %s\n", nbRequest->char_f);
        // connection with server
        char * abs_path_socket;
        if (find_absolute_path(nbRequest->char_f, &abs_path_socket) == -1) {
            exit(EXIT_FAILURE);
        }
        if (simple_opneConnection(abs_path_socket, 1, 10)== -1){
            fprintf(stderr, "errore while open connection to server\n");
            exit(EXIT_FAILURE);
        }

//        closeConnection(nbRequest->char_f);
//        exit(EXIT_FAILURE);

    }
    int cond = 1;
    printf("nb request : %d\n", nbRequest->tot_request);
    if (nbRequest->char_h) { // help
        PRINT_H
        cond = 0;
    }

    // while per iterare le richieste da inviare al server
    index = 0;
    while (cond || index < nbRequest->tot_request) {
        char options[2];
        strncpy(options, nbRequest->lst_char_abc[index]->option, 2);

        if (strcmp(options, "w") == 0) { // -w dirname [,n=0] scrivere tutto cio che sta nel dirname
            printf("char_w connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",=", &tmp); // token = dirname | tmp = n=0
            long nfile;
            if (tmp != NULL) { // controllo che ci sia un n
                token = strtok_r(tmp, "=,", &tmp); // token1 = n | tmp1 = 0
                nfile = strtol(tmp, NULL, 10);// setto la variabile che utilizzero' come max file possibili da scrivere
            } else nfile = 0;
            // controllo del path della directory
            char *dir_abspath;
            if (find_absolute_path(token, &dir_abspath) == -1) {
                printf("absolute path of dirname isn't correct\n");
                closeConnection(nbRequest->char_f);
                exit(EXIT_FAILURE);
            }
            // controllo che sia una directory
            struct stat st;
            int notused;
            CHECK_EXIT("stat directory\n", notused, stat(token, &st), -1)
            if (!S_ISDIR(st.st_mode)) {
                fprintf(stderr, "Non e' una directory\n");
                return EXIT_FAILURE;
            }
            char **files_to_write = calloc(nfile, sizeof(char));
            // salvo n file possibili nella lista che contiene i file che scrivero' nel server
            long nfiles = nfile;
            int indice = 0;
            recDirectory(dir_abspath, files_to_write, &nfiles, &indice);
            int i = 0;
            // devo fare un iterazione di diverse richieste per ogni pathname da far scrivere sul server
            while (i < nfile) {
                if (openFile(files_to_write[i], O_CREATE)==0) { // se file non sessiste e va creato
                    timer(nbRequest->char_t);
                    writeFile(files_to_write[i], NULL);
                } else if (openFile(files_to_write[i], O_OPEN)== 0) { // se file gia esistente
                    timer(nbRequest->char_t); // poiche openFile est una richiesta
                    FILE * f = fopen(files_to_write[i], "rb");
                    // Open the first text file
                    fseek(f, 0, SEEK_END);
                    long fsize = ftell(f);
                    fprintf(stderr, " size %ld\n", fsize);
                    fseek(f, 0, SEEK_SET);
                    // Read it into buffer
                    char * buf = calloc(fsize + 1, sizeof(char));
                    fread(buf, fsize, 1, f);
                    fclose(f);
                    buf[fsize] = 0;
                    printf("letto contenuto :\n %s\n", buf);
                    appendToFile(files_to_write[i], buf, fsize, nbRequest->lst_char_abc[index]->dirname);
                    free(buf);
                }
                timer(nbRequest->char_t);
                closeFile(files_to_write[i]);
                ++i;
                timer(nbRequest->char_t);
            }
            free(nbRequest->lst_char_abc[index]);
            ++index;
        }
        else if (strcmp(options, "W") == 0) { // -W file1[,file2]
            printf("char_W connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
            while (token) { // itero i file passati a cmd line, per scriverli sul file storage server
                char *abs_path_token;
                find_absolute_path(token, &abs_path_token);

                if (openFile(abs_path_token, O_CREATE)==0) { // se file non sessiste e va creato
                    timer(nbRequest->char_t);
                    writeFile(abs_path_token, NULL);
                    timer(nbRequest->char_t);
                    closeFile(abs_path_token);
                } else { // se file gia esistente
                    FILE * f = fopen(abs_path_token, "rb");
                    fseek(f, 0, SEEK_END);// setto il puntatore alla fine del file
                    long fsize = ftell(f);
                    fprintf(stderr, " size %ld\n", fsize);
                    fseek(f, 0, SEEK_SET);// setto il puntatore all'inizio del file
                    // Read it into buffer
                    char * buf = calloc(fsize + 1, sizeof(char));
                    fread(buf, fsize, 1, f);
                    fclose(f);
                    buf[fsize] = 0;
                    printf("letto contenuto :\n %s\n", buf);
                    appendToFile(abs_path_token, buf, fsize, nbRequest->lst_char_abc[index]->dirname);
                    free(buf);
                }
                timer(nbRequest->char_t);
                closeFile(abs_path_token);
                token = strtok_r(NULL, ",", &tmp);       // sequenza di righe che si ripete
                timer(nbRequest->char_t);
            }
            // liebro la memoria dalla richiesta, in modo da non dover far una richiesta dopo
            free(nbRequest->lst_char_abc[index]);
            ++index;
        }
        else if (strcmp(options, "r") == 0) { // -r file1[,file2] [-d dirname]
            printf("char_r connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
            void *buffer;
            int err = 0;
            size_t size_buffer;
            while (token || err == 0) {// // per iterare tutti i file da leggere nel file storage
                err = 0;
                char *abs_path_token;
                // estrapolo il path assoluto del file
                if (find_absolute_path((char*)token, &abs_path_token) == -1) {
                    printf("absolute path of filename [%s] isn't correct\n", token);
                    closeConnection(nbRequest->char_f);
                    err = 1;
                }
                if (err == 0 && openFile(abs_path_token, O_OPEN) == 0) {
                    if (readFile(abs_path_token, &buffer, &size_buffer) == -1) {
                        printf("errore nella lettura del file : %s\n", token);
                    } else {
                        char *copy_buffer = calloc(size_buffer, sizeof(char));
                        copy_buffer = buffer;
                        if (nbRequest->char_p) { // se -p attivo stampo i file letti da file storage richiesti
                            printf("-------------------------------");
                            printf("CONTENUTO FILE :\n %s\n%s\n\n", abs_path_token, copy_buffer);
                            printf("-------------------------------");
                        }
                        if (nbRequest->lst_char_abc[index]->dirname != NULL) {
                            char * filename = token;
                            char *base = basename(filename); // per essere sicura di prendere solo name
                            strncat(nbRequest->lst_char_abc[index]->dirname, base, strlen(base));
                            // creo un file per scriverci dentro alla directory giusta
                            FILE *f = fopen(nbRequest->lst_char_abc[index]->dirname, "wb");
                            fwrite(copy_buffer, size_buffer-1, 1, f);
                            fclose(f);
                        }
                    }
                }
                timer(nbRequest->char_t);
                if (err == 0) { // se pathname trovato allora faccio close
                    closeFile(abs_path_token);
                }
                token = strtok_r(NULL, ",", &tmp);
                timer(nbRequest->char_t);
            }
            free(nbRequest->lst_char_abc[index]);
            ++index;
        } else if (strcmp(options, "R") == 0) { // -R [n=0] [-d dirname]
            printf("char_R connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            int nb_files;
            if (nbRequest->lst_char_abc[index]->param != NULL) { // se n e' stato adto, allora tokenizzo
                unsigned long len = strlen(nbRequest->lst_char_abc[index]->param)-1;
                nb_files = nbRequest->lst_char_abc[index]->param[len] - '0';
                if (nb_files <= 0) nb_files = 0;
            } else {
                nb_files = 0;
            }
            readNFiles(nb_files, nbRequest->lst_char_abc[index]->dirname);
            free(nbRequest->lst_char_abc[index]);
            timer(nbRequest->char_t);// pausa fra una richiesta e l'altra
            ++index;
        } else if (strcmp(options, "c") == 0){
            printf("char_c connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
            while (token) { // itero i file passati a cmd line, per scriverli sul file storage server
                char *abs_path_token;
                if (find_absolute_path((char*)token, &abs_path_token) == -1) {
                    return -1;
                }
                removeFile(abs_path_token);
                token = strtok_r(NULL, ",", &tmp);
                timer(nbRequest->char_t);// pausa fra una richiesta e l'altra
            }
            free(nbRequest->lst_char_abc[index]);
            ++index;
        }



        // ------------------- DA Qui in poi da finire, aggiungere e modificare --------------------
        //timer(nbRequest->char_t);
    }


    // CONCLUSIONE
    fprintf(stderr, "\n");
    if (closeConnection(SOCKNAME) == -1) { // chiedo al server di chiudere la connessione con questo client
        fprintf(stderr, "Close connection error\n");
    } else {
        fprintf(stderr, "Close connection no problem\n");
    }
    //freer(&lst_char_abc, LST_SZ);
    fprintf(stderr, "\n");
    // il client chiude il suo canale lo fa direttamente deentro closeconnection
    //close(fd_c);
    return EXIT_SUCCESS;

}