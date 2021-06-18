
#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H


// struttura per le info su un determinato file
typedef struct node {
    long file_id;
    long file_sz;
    char * pathname;
    int fd_id; // int fd, quindi open || -1 close
    int modified;
    unsigned char * content_file;
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
} info_storage_t;

struct node *createFileNode(long key, int sz, char * name, int fd);


info_storage_t *createStorage(long ram, long nfile);


int removeFile(struct node **file_remove, int *nb_file_remove, info_storage_t **storage);


int searchFileNode(long key_file, info_storage_t * storage, struct node ** file_found);


int insertFileNode(struct node ** node_to_insert, info_storage_t **storage, int * nb_file_remove);


int removeSpecificFile(long key_file, info_storage_t ** storage, int * nb_file_remove);


int printStorage(info_storage_t * storage);


int freerStorage(info_storage_t ** storage);


#endif //FILE_STORAGE_H
