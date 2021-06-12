/*CLIENT*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
//#include <sys/types.h>
//#include <sys/un.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "util.h"
#include "API.h"
#include "configuration.h" // per checker il limite max che il storage puo' avere in numero di file e in byte

#define T_REQUEST 50// limite massimo di richieste possibili da parte di questo client

typedef struct {
    char option[2];
    char *param; // optarg, cio' ch sta dopo il comando
    char * dirname;
} command_t;

typedef struct {
    int tot_request;
    int char_h;
    int char_p;
    char* char_f;
    command_t ** lst_char_abc;
}nb_request;

void initLstRequest(nb_request ** lst_request) {
    CHECK_EXIT("calloc lst", *lst_request, calloc(1, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    //memset(*lst_request, 0, sizeof(nb_request));
    CHECK_EXIT("calloc lst", (*lst_request)->lst_char_abc, calloc(T_REQUEST, sizeof(nb_request)),NULL)// spazio suff. per freeare char_abc + msg
    memset((*lst_request)->lst_char_abc, 0, T_REQUEST* sizeof(nb_request));
    (*lst_request)->char_f = NULL;
    (*lst_request)->char_h = 0;
    (*lst_request)->char_p = 0;
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
    // estrapolo dal file config il limite massimo di file che posso inserire e il numero di bytes
    config_t *cfg = malloc(sizeof(config_t));
    configuration(argc, argv, &cfg);

    // connection with server
    simple_opneConnection(SOCKNAME, 1, 10);

    // Initialize char variable of each option
    nb_request * nbRequest;
    initLstRequest(&nbRequest);
    int index = 0;
    int opt;
    extern char *optarg;
    extern int optopt;
    int nb_request = 0;
    // ATTENZIONE ogni char puo avere piu richieste, generalizzare la ricezione delle informazioni per ogni opzione

    while ((opt = getopt(argc, argv, "p c:u:h l:t:d:R r:f:w:W")) != -1) {
        switch (opt) {
            case 'h':
                nbRequest->char_h = 1;
                break;
            case 'f': {
                nbRequest->char_f = optarg;
                break;
            }
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
                createRequest(optarg, NULL, "t", &nbRequest, &index);
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
        ++nbRequest->tot_request;
    }
    if (nbRequest->char_f == NULL) {
        nbRequest->char_f = SOCKNAME;
    }
    int cond = 1;
    printf("nb request : %d\n", nbRequest->tot_request);
    if (nbRequest->char_h) { // help
        PRINT_H
        cond = 0;
    } else if (nbRequest->char_f) { // sockanem
        printf("char_f connected with param : %s\n", nbRequest->char_f);
        if (simple_opneConnection(nbRequest->char_f, 1, 10)== -1){
            fprintf(stderr, "errore while open connection to server\n");
            cond = 0;
        }
        nbRequest->char_f = NULL;
    }
    // while per iterare le richieste da inviare al server
    index = 0;
    while (cond || index < nbRequest->tot_request) {
        char options[2];
        strncpy(options, nbRequest->lst_char_abc[index]->option, 2);
        if(strcmp(options, "w") == 0) { // dirname [,n=0] scrivere tutto cio che sta nel dirname
            printf("char_w connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
            char *tmp;
            char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp); // token = dirname | tmp = n=0
            long nfile;
            if (tmp != NULL) {
                char * tmp1;
                char * token1 = strtok_r(tmp, "=", &tmp1); // token1 = n | tmp1 = 0
                nfile = strtol(tmp1, NULL, 10);
                if (nfile> cfg->N_FILE) nfile = cfg->N_FILE;
                if(nfile == 0) nfile = cfg->N_FILE;
            } else nfile = cfg->N_FILE;
            // controllo del path della directory
            char * dir_abspath;
            if (find_absolute_path(token, &dir_abspath) == -1) {
                printf("absolute path of dirname isn't correct\n");
                closeConnection(nbRequest->char_f);
                exit(EXIT_FAILURE);
            }
            // controllo che sia una directory
            struct stat st;
            int notused;
            CHECK_EXIT("stat directory\n",notused,stat(token,&st), -1)
            if(!S_ISDIR(st.st_mode)) {
                fprintf(stderr, "Non e' una directory\n");
                return EXIT_FAILURE;
            }
            char **files_to_write = calloc(nfile, sizeof(char));
            // salvo n file possibili nella lista che contiene i file che scrivero' nel server
            long nfiles = nfile;
            long ram_size = cfg->MEM_SIZE;
            int indice = 0;
            recDirectory(dir_abspath, files_to_write, &nfiles, &ram_size, &indice);
            int i = 0;
            while (i < nfile) {
                // devo fare un iterazione di diverse richiesteper ogni pathname, che sono messi in una lst
                writeFile(files_to_write[i], NULL);// pathname, dirname
                ++i;
            }
            ++nbRequest->tot_request;
        }

            // ------------------- DA Qui in poi da finire, aggiungere e modificare --------------------

            if (strcmp(options, "W") == 0) { //
                printf("char_W connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
                // stessa cosa di sopra ma con file al posto di n
                writeFile(NULL, nbRequest->lst_char_abc[index]->param);
                ++nbRequest->tot_request;
            }



            if (strcmp(options, "r") == 0) {
                printf("char_r connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
                // fare una lista dei file da leggere da server
                char ** lst_files_read; // lst per poter inseriure file nel dirname in un n secondo momento
                // !!!!!!!!!!!!!!!!!!! NON RICORDO PIU COME SI ALLOCA MEMORIA PER UN Char** !!!!!!!!!!!!!!!!!!!!!!!
                //CHECK_EXIT("calloc lst files", lst_files_read, calloc(1, sizeof(char)), NULL)
                char *tmp;
                char *token = strtok_r(nbRequest->lst_char_abc[index]->param, ",", &tmp);
                void * buffer;
                size_t size_buffer;
                while (token) {// while per inserire i
                    token = strtok_r(NULL, " ", &tmp);
                    if (readFile(nbRequest->lst_char_abc[index]->param, &buffer, &size_buffer) == 0) {
                        // mettere il buffer nella lista
                    }
                }

            }
            if(strcmp(options, "R") == 0) {
                printf("char_R connected with param : %s\n", nbRequest->lst_char_abc[index]->param);
                long nb_files;
                if(nbRequest->lst_char_abc[index]->param != NULL) {
                    char *tmp;
                    char *token = strtok_r(nbRequest->lst_char_abc[index]->param, "=", &tmp);
                    void * buffer;
                    size_t size_buffer;
                    nb_files = strtol(tmp, NULL, 10);
                } else nb_files = -1;

                //else { c }

//                if (readNFiles(nb_files, )) {
//                    printf("errore nel readNFiles\n");
//                }
            }

            if (nb_request <= 0) {
                cond = 0;
            }
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
    close(fd_c); // il client chiude il suo canale
    return EXIT_SUCCESS;

}