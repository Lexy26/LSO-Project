
#ifndef FILE_STORAGE_H
#define FILE_STORAGE_H

#include <unistd.h>
#include "util.h"

typedef struct node {
    long file_id;
    long file_sz;
    char * pathname;
    int is_open; // 1 yes | 0 no
    struct node *son;
    struct node *father;

} node_t;

typedef struct {
    long ram_tot;
    long ram_dispo;
    long nfile_tot;
    long nfile_dispo;
    struct node *head;
    struct node *last;

} info_storage_t;

static inline struct node *createFileNode(long key, int sz, char * name) {
    // controllo size del file per vedere se entra in bytes nella memoria
    // controlla che c'e' spazio a livello di nfile
    struct node *new_node;
    CHECK_EXIT("calloc new node", new_node, calloc(1, sizeof(struct node)), NULL)
//    struct stat st;
//    stat(name, &st);
//    long size = st.st_size;

    new_node->file_id = key;
    new_node->file_sz = sz;
    new_node->is_open = 0;
    new_node->son = NULL;
    new_node->father = NULL;
    new_node->pathname = name;
    return new_node;
}

static inline info_storage_t *createStorage(long ram, long nfile) {

    info_storage_t *storage_init;
    CHECK_EXIT("calloc storage init", storage_init, calloc(1, sizeof(info_storage_t)), NULL)

    storage_init->head = NULL;
    storage_init->last = NULL;

    storage_init->nfile_dispo = nfile;
    storage_init->nfile_tot = nfile;

    storage_init->ram_dispo = ram;
    storage_init->ram_tot = ram;

    printf("Storage create \n");

    return storage_init;
}

static inline int removeFile(struct node **file_remove, int *nb_file_remove, info_storage_t **storage) {//First in Last out
    if (!(*file_remove)->is_open) {// se il file non e' aperto, allora lo posso togliere dallo storage
        printf("file non aperto posso procedere...\n");
        struct node *tmp_father = (*file_remove)->father;
        struct node *tmp_son = (*file_remove)->son;
        struct node *tmp = *file_remove;
        if (tmp_son == NULL) { // remove e aggiorno last file
            printf("il nodo est LAST\n");
            if (tmp_father == NULL) {
                (*storage)->head = NULL;
                (*storage)->last = NULL;
            } else {
                tmp_father->son = NULL;
                (*storage)->last = tmp_father;
            }
        } else {
            if (tmp_father == NULL) {
                printf("e' la HEAD\n");
                (*storage)->head = tmp_son;
                tmp_son->father = NULL;
            } else {
                printf("e' un MIDDLE\n");
                // se un file sta in mezzo
                tmp_father->son = (*file_remove)->son;
                tmp_son->father = (*file_remove)->father;
                printf("rimosso\n");
            }
        }
        // aggiorno ram, numero di file e il counter di file rimossi
        *nb_file_remove += 1;
        (*storage)->ram_dispo += tmp->file_sz;
        (*storage)->nfile_dispo += 1;
        free(tmp);
        return 0;
    }
    return -1;
}

static inline int searchFileNode(long key_file, info_storage_t * storage, struct node ** file_found){
    // search file a specific file in storage
    long nb_file_check = storage->nfile_tot-storage->nfile_dispo;
    printf("file to check : %ld\n", nb_file_check);
    struct node * current = storage->head;
    while (nb_file_check != 0) {
        if (current->file_id == key_file){
            *file_found = current;
            return 0;
        } else {
            current = current->son;
        }
        nb_file_check -= 1;
    }
    return -1;
}

static inline int insertFileNode(struct node ** node_to_insert, info_storage_t **storage, int * nb_file_remove) { // First in Last out
    if(searchFileNode((*node_to_insert)->file_id, *storage, node_to_insert) == 0){
        printf("file gia esistente\n");
        return -1;
    } else {
        if((*storage)->head == NULL && (*storage)->last == NULL) { // unico nodo file nello storage
            printf("primo nodo inserito\n");
            (*storage)->head = *node_to_insert;
            (*storage)->last = *node_to_insert;
            // aggiorna disponibilita' dello spazio in memoria
            (*storage)->ram_dispo -= (*node_to_insert)->file_sz;
            (*storage)->nfile_dispo -= 1;
            //printf("file dispo : %ld\n", (*storage)->nfile_dispo);
        } else {
            printf("else\n");
            long ram_used = (*storage)->ram_dispo - (*node_to_insert)->file_sz <= 0;
            if ( ram_used || (*storage)->nfile_dispo == 0) {// se spazio e' esaurito allora entra nell'if
                printf("remove file necessario\n");
                struct node *removable = (*storage)->last;
                while (ram_used && (*storage)->nfile_dispo == 0) {
                    removable = (*storage)->last;
                    while (removable->father != NULL) {
                        if (removeFile(&removable, nb_file_remove, storage) == -1) { // se problema con la rimozione (file open)
                            removable = removable->father; // check allora se un'altro file non est in stato aperto
                        }
                    } // se tutti i file sono aperti rifaccio la ricerca di file chiusi == volendo potrei usare i thread con wait
                }
            }
            printf("inserimento nodo\n");
            struct node *tmp_head = (*storage)->head;
            (*storage)->head = *node_to_insert;//aggiorno la head ptr dello storage col nuovo file
            (*node_to_insert)->son = tmp_head;
            tmp_head->father = *node_to_insert;
            // aggiorna disponibilita' dello spazio in memoria
            (*storage)->ram_dispo -= (*node_to_insert)->file_sz;
            (*storage)->nfile_dispo -= 1;
            //printf("file dispo : %ld\n", (*storage)->nfile_dispo);

        }
    }

    return 0;
}

static inline int removeSpecificFile(long key_file, info_storage_t ** storage, int * nb_file_remove) {
    // se voglio rimuovere uno specifico file
    struct node * find_file;
    if (searchFileNode(key_file, *storage, &find_file) == 0) {
        printf("search file FOUND\n");
        printf("is open : %d\n", find_file->is_open);
        if (find_file->is_open == 1) {
            while (find_file->is_open == 1) {
                printf("waiting...\n");
                sleep(1);
            }
        }
        removeFile(&find_file, nb_file_remove, storage);
        return 0;
    }
    return -1;
}

static inline int printStorage(info_storage_t * storage) {
    printf("----------------\n");
    struct node * current = storage->head;
    while(current != NULL) {
        printf("info : %s\n", current->pathname);
        current = current->son;
    }
    printf("-----------------\n");
    return 0;
}

static inline int freerStorage(info_storage_t ** storage) {
    long nb_file_check = (*storage)->nfile_tot-(*storage)->nfile_dispo;

    struct node * current = (*storage)->head;
    while (nb_file_check != 0) {
        struct node * tmp = current;
        current = current->son;
        free(tmp);
        printf("file free : %ld\n", nb_file_check);
        nb_file_check -= 1;
    }
    free(*storage);
    return 0;
}
#endif //FILE_STORAGE_H
