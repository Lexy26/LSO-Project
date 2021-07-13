
#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#define TIMER 30
#define SLEEP_TIME 0.0001

// struttura per le info su un determinato file
typedef struct node {
    long file_sz; // the size in bytes of the content of the file
    char * pathname; // absolute path of file, key id
    int fdClient_id; // if there is a fd client, the file is open, otherwise is close (-1)
    int modified; // if modified=-1 the file is inserted but not filled, otherwise it is filled (1)
    unsigned char * content_file;
//    unsigned char * init_pointer_file;
    struct node *son;
    struct node *father;
} node_t;

// struttura per le info dello storage
typedef struct {
    long ram_tot;
    long ram_dispo;
    long nfile_tot;
    long nfile_dispo;
    struct node *head;
    struct node *last;
    pthread_mutex_t lock;
} info_storage_t;

struct node *createFileNode(char * pathname, int fd);


info_storage_t *createStorage(long ram, long nfile);


int removeFile(struct node **file_remove, char ** pathname_removed, info_storage_t **storage, struct node ** current_file, int *nfile_removed, FILE * logfile);


int searchFileNode(char * pathname, info_storage_t ** storage, struct node ** file_found);


int insertCreateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int *nfile_removed, FILE * logfile);


int UpdateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int fd_current, long size_buf, unsigned char * content, int * nfile_removed, FILE * logfile);


void printStorage(info_storage_t ** storage, FILE * logfile);


void freerStorage(info_storage_t ** storage);


#endif //FILE_STORAGE_H
