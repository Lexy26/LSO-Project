/*CLIENT*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
//#include <sys/socket.h>
#include <getopt.h>
//#include <sys/types.h>
//#include <sys/un.h>
#include <errno.h>
#include <string.h>

#include "util.h"
#include "API.h"

//#define BUFSIZE 256
#define LST_SZ 15

typedef struct {
    int option; // 1 se comando c'e' || 0 se comqndo non richiesto
    char *param; // optarg, cio' ch sta dopo il comando
} command_t;

void initCommand(command_t **char_abc) {
    CHECK_EXIT("calloc", *char_abc, calloc(1, sizeof(command_t)), NULL)
    //CHECK_EXIT("calloc", (*char_abc)->param, calloc(BUFSIZE, sizeof(char)), NULL)
    (*char_abc)->option = 0;
    (*char_abc)->param = NULL;
}

void upgradeCommand(command_t **char_abc, command_t **lst, int index, char *ptarg) {
    (*char_abc)->option = 1;
    (*char_abc)->param = ptarg;
    lst[index] = *char_abc;
}

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

    // connection with server
    simple_opneConnection(SOCKNAME, 1, 10);

    // Initialize char variable of each option
    command_t *char_h, *char_r, *char_R, *char_p, *char_c, *char_u, *char_l, *char_t, *char_d, *char_f, *char_w, *char_W;
    command_t **lst_char_abc;
    CHECK_EXIT("calloc lst", lst_char_abc, calloc(LST_SZ, sizeof(command_t)),NULL)// spazio suff. per freeare char_abc + msg
    memset(lst_char_abc, 0, LST_SZ);
    int i = 0;
    int opt;
    extern char *optarg;
    extern int optopt;
    initCommand(&char_h);
    initCommand(&char_r);
    initCommand(&char_R);
    initCommand(&char_p);
    initCommand(&char_c);
    initCommand(&char_u);
    initCommand(&char_l);
    initCommand(&char_t);
    initCommand(&char_d);
    initCommand(&char_f);
    initCommand(&char_W);
    initCommand(&char_w);

    while ((opt = getopt(argc, argv, "p c:u:h l:t:d:R:r:f:w:W")) != -1) {
        switch (opt) {
            case 'h':
                upgradeCommand(&char_h, lst_char_abc, i, optarg);
                break;
            case 'r':
                upgradeCommand(&char_r, lst_char_abc, i, optarg);
                break;
            case 'R':
                upgradeCommand(&char_R, lst_char_abc, i, optarg);
                break;
            case 'p':
                printf("connect p : %s\n", optarg);
                upgradeCommand(&char_p, lst_char_abc, i, optarg);
                break;
            case 'c':
                printf("connect c : %s\n", optarg);
                upgradeCommand(&char_c, lst_char_abc, i, optarg);
                break;
            case 'u':
                printf("connect u : %s\n", optarg);
                upgradeCommand(&char_u, lst_char_abc, i, optarg);
                break;
            case 'l':
                printf("connect l : %s\n", optarg);
                upgradeCommand(&char_l, lst_char_abc, i, optarg);
                break;
            case 't':
                printf("connect t : %s\n", optarg);
                upgradeCommand(&char_t, lst_char_abc, i, optarg);
                break;
            case 'd':
                printf("connect d : %s\n", optarg);
                upgradeCommand(&char_d, lst_char_abc, i, optarg);
                break;
            case 'f':
                printf("connect f : %s\n", optarg);
                upgradeCommand(&char_f, lst_char_abc, i, optarg);
                break;
            case 'W':
                printf("connect W : %s\n", optarg);
                upgradeCommand(&char_W, lst_char_abc, i, optarg);
                break;
            case 'w':
                printf("connect w : %s\n", char_w->param);
                upgradeCommand(&char_w, lst_char_abc, i, optarg);
                break;
//            case ':': {
//                printf("l'opzione '-%c' richiede un argomento\n", optopt);
//            }
//                break;
            case '?': /* '?' */
            {
                fprintf(stderr, "-%c : comando errato\n", optopt);
                break;
            }
            default:
                printf("default\n");
        }
        ++i;
    }
    int cond = 1;
    while (cond) {
        if (char_h->option) {
            PRINT_H
            char_h->option = 0;
        } else {
            if (char_r->option) {
                printf("char_r connected with param : %s\n", char_r->param);
                char_r->option = 0;
            } else if (char_W->option) {
                printf("char_W connected with param : %s\n", char_W->param);
                char_W->option = 0;
            } else if (char_c->option) {
                printf("char_c connected with param : %s\n", char_c->param);
                char_c->option = 0;
            } else if (char_d->option) {
                if (char_r->option || char_R->option) {
                    printf("r o R con d\n");
                } else {
                    exit(EXIT_FAILURE);
                }
            } else {
                cond = 0;
            }
        }
    }
    fprintf(stderr, "\n");
    if (closeConnection(SOCKNAME) == -1) { // chiedo al server di chiudere la connessione con questo client
        fprintf(stderr, "Close connection error\n");
    } else {
        fprintf(stderr, "Close connection no problem\n");
    }
    freer(lst_char_abc, LST_SZ);
    fprintf(stderr, "\n");
    close(fd_c); // il client chiude il suo canale
    return EXIT_SUCCESS;

}