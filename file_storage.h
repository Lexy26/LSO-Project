
#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#define TIMER 10
#define SLEEP_TIME 0.0001

// struttura per le info su un determinato file
typedef struct node {
    //long file_id;
    long file_sz;
    char * pathname;
    int fdClient_id; // int fd, quindi open || -1 close
    int modified;
    unsigned char * content_file;
    unsigned char * init_pointer_file;
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

struct node *createFileNode(char * pathname, int fd); // long key


info_storage_t *createStorage(long ram, long nfile);


int removeFile(struct node **file_remove, char ** pathname_removed, info_storage_t **storage, struct node ** current_file);


int searchFileNode(char * pathname, info_storage_t ** storage, struct node ** file_found);


int insertCreateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path);


int UpdateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int fd_current, long size_buf, unsigned char * content);


int removeSpecificFile(char * pathname, info_storage_t ** storage, char ** pathname_removed);


int printStorage(info_storage_t ** storage);


int freerStorage(info_storage_t ** storage);


#endif //FILE_STORAGE_H
