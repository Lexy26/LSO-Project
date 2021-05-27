/*CLIENT*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <errno.h>
#include <string.h>

#define SOCKNAME "./cs_sock"
#define BUFSIZE 256

typedef struct message {
    int len;
    char *str;
} msg_t;

typedef struct {
    int command;
    char which[2];
    char * contenuto;
}instr_t;

int main(int argc, char *argv[]) {
    // fare un warning sul controllo del PATH ASSOLUTO, il quale non viene dato da riga
    // di comando, lo devo controllare io nel server
    // analizzare con getopt il numero di richieste fatte dal client in linea di comando
    // faccio un 'for' da protocollo richiesta risdposta
    if (argc < 2) {
        printf("usage: ./client <comand1> <comand2>\n");
        exit(EXIT_FAILURE);
    }
    int fd_c; // crea client socket + error control

    if ((fd_c = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {  // creo il socket
        perror("socket");
        exit(errno);
    }

    struct sockaddr_un serv_addr; // setto l'indirizzo
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKNAME,
            strlen(SOCKNAME) + 1);// do' il nome al file descr del socket per la connection

    // creo connessione col server
    if (connect(fd_c, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        exit(errno);
    }

    msg_t messagio;// alloca spazio per il msg + error control
    if ((messagio.str = malloc(BUFSIZE)) == NULL) {
        perror("malloc");
        exit(errno);
    }
    instr_t *char_h = malloc(sizeof(instr_t));

    int char_r = 0, char_R = 0, char_p = 0, char_c = 0, char_u = 0, char_l = 0, char_t = 0, char_d = 0, char_f = 0, char_w = 0, char_W = 0;
    int opt;
    while ((opt = getopt(argc, argv, "p:c:u:h l:t:d:R:r:f:w:W")) != -1) { // salta quelle parti che non sono '-n'
        switch (opt) {
            case 'h':
                char_h->command = 1;
                printf("connect h\n");
                break;
            /* ---- under construction init ----*/
            case 'r':
                printf("connect r\n");
                char_r = 1;
                break;
            case 'R':
                printf("connect R");
                char_R = 1;
                break;
            case 'p':
                printf("connect p");
                char_p = 1;
                break;
            case 'c':
                printf("connect c");
                char_c = 1;
                break;
            case 'u':
                printf("connect u");
                char_u = 1;
                break;
            case 'l':
                printf("connect l");
                char_l = 1;
                break;
            case 't':
                printf("connect t");
                char_t = 1;
                break;
            case 'd':
                printf("connect d");
                char_d = 1;
                break;
            case 'f':
                printf("connect f");
                char_f = 1;
                break;
            case 'W':
                printf("connect W");
                char_W = 1;
                break;
            case 'w':
                printf("connect w");
                char_w = 1;
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
            /* ---- code under construction fine ----*/
        }
    }

    if (char_h->command) { // test of connection with server
        messagio.str = "from Client : Usage: ./client -<comando1> <args> ...\n";
        messagio.len = (int) strlen(messagio.str);
        write(fd_c, &messagio.len, sizeof(int));
        write(fd_c, messagio.str, messagio.len);
        printf("message wirtten to server\n");
        int sz;
        char buffer[BUFSIZE];
        read(fd_c, &sz, sizeof(int));
        read(fd_c, buffer, sz);
        printf("message ricieve : %s\n", buffer);
        return EXIT_SUCCESS;
    } else {
        printf("other\n");
    }

//    close(fd_c); // il client chiude il suo canale
//    exit(EXIT_SUCCESS);

}