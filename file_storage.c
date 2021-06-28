/* ------------------ FILE STORAGE SERVER -------------------- */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "file_storage.h"
//#include "util_server.h"

// Inizializza la struttura del nodo
struct node *createFileNode(char * name, int fd) {
    struct node *new_node = malloc(sizeof(struct node));
    memset(new_node, 0, sizeof(struct node));
    //CHECK_EXIT("calloc new node", new_node, calloc(1, sizeof(struct node)), NULL)
//    struct stat st;
//    stat(name, &st);
//    long size = st.st_size;
    new_node->file_sz = 0;//in byte usando stat
    new_node->fdClient_id = fd;
    new_node->son = NULL;
    new_node->father = NULL;
    new_node->pathname = name;
    new_node->content_file = malloc(sizeof(unsigned char));
    memset(new_node->content_file, 0, sizeof(unsigned char));
    new_node->init_pointer_file = new_node->content_file;
    new_node->modified = 0;
    return new_node;
}

// inizializza lo storage
info_storage_t *createStorage(long ram, long nfile) {

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


int removeFile(struct node **file_remove, char ** file_removed, info_storage_t **storage) {//First in Last out
    // se il file non e' aperto oppure est stato modificato almeno una volta, allora lo posso togliere dallo storage
    if ((*file_remove)->fdClient_id == -1 || (*file_remove)->modified != 0) {
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
        *file_removed = tmp->pathname;
        (*storage)->ram_dispo += tmp->file_sz;
        (*storage)->nfile_dispo += 1;
        free(tmp);
        return 0;
    }
    return -1;
}


int searchFileNode(char * pathname, info_storage_t * storage, struct node ** file_found) {
    // search file a specific file in storage
    //printf("tot : %ld, dispo : %ld\n", storage->nfile_tot, storage->nfile_dispo);
    long nb_file_check = storage->nfile_tot-storage->nfile_dispo;
    printf("file to check : %ld\n", nb_file_check);
    struct node * current = storage->head;
    while (nb_file_check != 0) {
        if (strncmp(current->pathname, pathname, strlen(pathname))==0){
            //printf("trovato !!\n");
            *file_found = current;
            return 0;
        } else {
            current = current->son;
        }
        nb_file_check -= 1;
    }
    return -1;
}

//PROBLEMA del CRONTROLLO in Bytes dei file con ram disponibile
int insertFileNode(struct node ** node_to_insert, info_storage_t **storage, char ** pathname_removed) { // First in Last out
    // controllo size del file per vedere se entra in bytes nella memoria
    // controlla che c'e' spazio a livello di nfile

    if((*storage)->head == NULL && (*storage)->last == NULL) { // unico nodo file nello storage
        printf("primo nodo inserito\n");
        (*storage)->head = *node_to_insert;
        (*storage)->last = *node_to_insert;
        // aggiorna disponibilita' dello spazio in memoria
        (*storage)->ram_dispo -= (*node_to_insert)->file_sz;
        (*storage)->nfile_dispo -= 1;
    } else {
        printf("else\n");
        long ram_used = (*storage)->ram_dispo - (*node_to_insert)->file_sz <= 0;
        if ( ram_used || (*storage)->nfile_dispo == 0) {// se spazio e' esaurito allora entra nell'if
            printf("remove file necessario\n");
            struct node *removable = (*storage)->last;
            while (ram_used && (*storage)->nfile_dispo == 0) {
                removable = (*storage)->last;
                while (removable->father != NULL) { // qui devo aggiungere il punto di realloc per registrare il path dei file perduti da pandare al client
                    if (removeFile(&removable, pathname_removed, storage) == -1) { // se problema con la rimozione (file open)
                        removable = removable->father; // check allora se un'altro file non est in stato aperto
                    } else {
                        printf("path name removed : %s \n", *pathname_removed);
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
    }
    return 0;
}

int updateFileNode(info_storage_t **storage, unsigned char * content, long size_buf, struct node ** node_to_insert, int fd_current) { // ,sms_arg * msg_content
    if (fd_current == (*node_to_insert)->fdClient_id){
        // funzione che rimuove file se spazio insufficiente

        (*node_to_insert)->modified = 1;
        (*node_to_insert)->content_file = realloc((*node_to_insert)->content_file, size_buf);
        (*node_to_insert)->init_pointer_file = (*node_to_insert)->content_file;
        memcpy((*node_to_insert)->content_file + (*node_to_insert)->file_sz, content, size_buf);
        (*node_to_insert)->file_sz += size_buf;
        (*storage)->ram_dispo -= (*node_to_insert)->file_sz;
    }
    return 0;

}

int removeSpecificFile(char * pathname, info_storage_t ** storage, char ** pathname_removed) {
    // se voglio rimuovere uno specifico file
    struct node * find_file;
    if (searchFileNode(pathname, *storage, &find_file) == 0) {
        printf("search file FOUND\n");
        printf("is open : %d\n", find_file->fdClient_id);
        if (find_file->fdClient_id != -1) {
            while (find_file->fdClient_id != 1) {
                printf("waiting...\n");
                sleep(1);
            }
        }
        removeFile(&find_file, pathname_removed, storage);
        return 0;
    }
    return -1;
}


int printStorage(info_storage_t * storage) {
    // Dice cosa contiene lo storage
    printf("+++++++++++++++++++++++++++++\n");
    struct node * current = storage->head;
    while(current != NULL) {
        printf("info : %s\nfd_id : %d\n", current->pathname, current->fdClient_id);
        current = current->son;
    }
    printf("+++++++++++++++++++++++++++++\n");
    return 0;
}


int freerStorage(info_storage_t ** storage) {
    // libero la memoria --------- FUNZIONE da DEFINIRE
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