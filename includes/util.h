#ifndef UTIL_H
#define UTIL_H

#if !defined(BUFSIZE)
#define BUFSIZE 256
#endif

// message structure client-server communication
typedef struct {
    size_t len;
    unsigned char *str;
} msg_t;

// structure for command information
typedef struct {
    char option[2];
    char *param; // optarg, cio' che sta dopo l'opzione opt
    char * dirname;
} command_t;

// structure to list requests
typedef struct {
    int tot_request;
    int char_t;
    int char_h;
    int char_p;
    char* char_f;
    command_t ** lst_char_abc;
}nb_request;

// print per il file di log
#define LOG_PRINT(file, content) fprintf(file, "%s | %s ==== %s\n", __DATE__, __TIME__,content);

#define LOG_PRINT2_INT(file, content1, content2) fprintf(file, "%s | %s ==== %s %d\n", __DATE__, __TIME__, content1, content2);

// SYSCALL Control
#define CHECK_NEQ_EXIT(name, sc, check)    \
    if ((sc) != (check)) {                \
    perror(#name);                \
    exit(errno);            \
    }

#define CHECK_EQ_EXIT(name, sc, check)    \
    if ((sc) == (check)) {                \
    perror(#name);                \
    exit(errno);            \
    }

#define CHECK_EXIT_VAR(name, var, sc, check)    \
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

#define LOCK(mutex, res) \
    if (pthread_mutex_lock(mutex)!=0) { \
        fprintf(stderr, "LOCK Error\n");  \
        return res;      \
            }   \

#define UNLOCK(mutex, res) \
    if (pthread_mutex_unlock(mutex)!=0) { \
        fprintf(stderr, "UNLOCK Error\n");		    \
        return res;			    \
  }

void sendMsg_File_Content(int fd_c, char *api_id, char *arg1, char * arg2, unsigned char* arg3);


void sendMsg(int fd_c, char api_id[3], char *arg1, char * arg2);


void receivedMsg_File_Content(char ** pathname, unsigned char** sms_content, size_t * size_buf,int * check, int fd_c);


void recievedMsg(unsigned char ** message,int fd_c);

// Output of -h command
#define PRINT_H printf("\nFile Storage Server - Progetto di Laboratorio di Sistemi Operativi 2020/2021\n");\
printf("\nusage: ./client options [parameters]\n");\
printf("\n OPTIONS:\n   -h \t\t\t\t\tStampa la lista di tutte le opzioni accettate dal client\n");\
printf("\n   -f filename\t\t\t\tSpecifica il nome del socket AF_UNIX a cui connettersi\n");\
printf("\n   -w dirname[,n=0]   \t\t\tInvia al server i file nella cartella ‘dirname’ "\
"\n\t\t\t\t\tse n=0 (o non è specificato) non c’è un limite superiore al numero di file da"\
"\n\t\t\t\t\tinviare al server (tuttavia non è detto che il server possa scriverli tutti)\n");\
printf("\n   -W file1[,file2]   \t\t\tLista di nomi di file da scrivere nel server separati da ‘,’\n");\
printf("\n   -r file1[,file2]   \t\t\tLista di nomi di file da leggere dal server separati da ‘,’\n");\
printf("\n   -R n=0     \t\t\t\tTale opzione permette di leggere ‘n’ file qualsiasi attualmente memorizzati nel server"\
"\n\t\t\t\t\tse n=0 (o non è specificato) allora vengono letti tutti i file presenti nel server\n");\
printf("\n   -d dirname \t\t\t\tCartella in memoria secondaria dove scrivere i file letti dal server con l’opzione ‘-r’ o ‘-R’\n");\
printf("\n   -t time\t\t\t\tTempo in millisecondi che intercorre tra l’invio di due richieste successive al server (se non"\
"\n\t\t\t\t\tspecificata si suppone -t 0, cioè non c’è alcun ritardo tra l’invio di due richieste consecutive)\n");\
printf("\n   -l file1[,file2]   \t\t\tLista di nomi di file su cui acquisire la mutua esclusione\n");\
printf("\n   -u file1[,file2]   \t\t\tLista di nomi di file su cui rilasciare la mutua esclusione\n");\
printf("\n   -c file1[,file2]   \t\t\tLista di file da rimuovere dal server se presenti\n");\
printf("\n   -p \t\t\t\t\tAbilita le stampe sullo standard output per ogni operazione\n\n\n");\

#endif //UTIL_H
