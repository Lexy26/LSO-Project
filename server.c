/*-------------------FILE STORAGE SERVER-------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>


#define BUF_SIZE 256

typedef struct {
    int N_THREAD; // number of threads in server
    int N_FILE; // max number of possible file in storage
    int MEM_SIZE; //max size of memory storage in Mbytes
} config_t;

int main(int argc, char *argv[]) {
    if (argc != 3) {// ~$ ./server -F config.txt (null)
        printf("Use: ./server -F <fileConfig.txt>\n");
        return -1;
    }
    FILE *config = NULL;
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
    char buffer[BUF_SIZE]; // qui e' dove mettero' le righe del file config.txt
    config_t cfg;
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
                cfg.N_THREAD = (int) number;
                printf("N_THREAD cfg %d\n", cfg.N_THREAD);
            } else {
                printf("N_THREAD convertion error");
                exit(EXIT_FAILURE);
            }
        } else if (strncmp(token, "N_FILE", strlen("N_FILE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);// trasformo il num sringa in int.
            if (correct != NULL && *correct == (char) 0) {
                cfg.N_FILE = (int) number;
                printf("N_FILE cfg %d\n", cfg.N_FILE);
            } else {
                perror("N_FILE convertion error");
                exit(EXIT_FAILURE);
            }
        } else if (strncmp(token, "MEM_SIZE", strlen("MEM_SIZE")) == 0) {
            token = strtok_r(NULL, " ", &tmp);
            long number = strtol(token, &correct, 10);// trasformo il num sringa in int.
            if (correct != NULL && *correct == (char) 0) {
                cfg.MEM_SIZE = (int) number;
                printf("MEM_SIZE cfg %d\n", cfg.MEM_SIZE);
            } else {
                perror("MEM_SIZE convertion error");
                exit(EXIT_FAILURE);
            }
        } else {// se per caso il formato del file config non e' corretto allora errore
            printf("format config file error\n");
            fprintf(stderr, "Value of errno: %d\n", errno);
        }
    }
    fclose(config);
    return 0;
}
