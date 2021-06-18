#ifndef UTIL_H
#define UTIL_H

#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

#if !defined(SOCKNAME)
#define SOCKNAME "./cs_sock"
#endif

// struct messaggio utile per far comunicare client-server
typedef struct {
    size_t len;
    unsigned char *str;
} msg_t;

// struct utile al client
typedef struct {
    char option[2];
    char *param; // optarg, cio' che sta dopo l'opzione
    char * dirname;
} command_t;

// struct utile al client
typedef struct {
    int tot_request;
    int char_t;
    int char_h;
    int char_p;
    char* char_f;
    command_t ** lst_char_abc;
}nb_request;

// check del valore con uscita
#define CHECK_EXIT(name, var, sc, check)    \
    if (((var)=(sc)) == (check)) {                \
    perror(#name);                \
    exit(errno);            \
    }
// check del valore senza uscita
#define CHECK_RETURN(name, var, sc, check)    \
    if (((var)=(sc)) == (check)) {                \
    perror(#name);                            \
    int err = errno;                          \
    errno = err;\
    }
int sendMsg_ClientToServer_Append(int fd_c, char api_id[3], char *arg1, char * arg2, unsigned char* arg3);

int sendMsg_ClientToServer(int fd_c, char api_id[3], char *arg1, char * arg2);

int sendMsg_ServerToClient(int fd_client,unsigned char * arg1, unsigned char * arg2, unsigned char* arg3);

int isdot(const char dir[]);

int recDirectory(char * dirname, char ** lst_of_files, long *nfiles, int *index);

int find_absolute_path(char* pathname, char **abs_path);

void recievedMsg_ServerToClient(unsigned char ** messaggio,int fd_c);

void recievedMsg_ServerToClient_Read(char ** pathname, unsigned char** sms_content, size_t * size_buf,int * check, int fd_c);
//int writeCommand(nb_request **pNbRequest, char * filename);

void recievedMsg_ClientToServer(unsigned char ** sms_info, int fd_c);

// cio' che -h stampera' in stdout
#define PRINT_H printf("\nFile Storage Server - Progetto di Laboratorio di Sistemi Operativi 2020/2021\n");\
printf("\nusage: ./client options [parameters]\n");\
printf("\n OPTIONS:\n   -h \t\t\t\t\tStampa la lista di tutte le opzioni accettate dal client\n");\
printf("\n   -f filename\t\t\t\tSpecifica il nome del socket AF_UNIX a cui connettersi\n");\
printf("\n   -w dirname[,n=0]   \t\t\tInvia al server i file nella cartella ‘dirname’ "\
"\n\t\t\t\t\tse n=0 (o non è specificato) non c’è un limite superiore al numero di file da"\
"\n\t\t\t\t\tinviare al server (tuttavia non è detto che il server possa scriverli tutti)\n");\
printf("\n   -W file1[,file2]   \t\t\tLista di nomi di file da scrivere nel server separati da ‘,’\n");\
printf("\n   -r file1[,file2]   \t\t\tLista di nomi di file da leggere dal server separati da ‘,’\n");\
printf("\n   -R [n=0]   \t\t\t\tTale opzione permette di leggere ‘n’ file qualsiasi attualmente memorizzati nel server"\
"\n\t\t\t\t\tse n=0 (o non è specificato) allora vengono letti tutti i file presenti nel server\n");\
printf("\n   -d dirname \t\t\t\tCartella in memoria secondaria dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’\n");\
printf("\n   -t time\t\t\t\tTempo in millisecondi che intercorre tra l’invio di due richieste successive al server (se non"\
"\n\t\t\t\t\tspecificata si suppone -t 0, cioè non c’è alcun ritardo tra l’invio di due richieste consecutive)\n");\
printf("\n   -l file1[,file2]   \t\t\tLista di nomi di file su cui acquisire la mutua esclusione\n");\
printf("\n   -u file1[,file2]   \t\t\tLista di nomi di file su cui rilasciare la mutua esclusione\n");\
printf("\n   -c file1[,file2]   \t\t\tLista di file da rimuovere dal server se presenti\n");\
printf("\n   -p \t\t\t\t\tAbilita le stampe sullo standard output per ogni operazione\n\n\n");\

#endif //UTIL_H
