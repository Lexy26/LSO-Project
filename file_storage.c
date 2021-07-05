/* ------------------ FILE STORAGE SERVER -------------------- */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "util.h"
#include "file_storage.h"

#define test 0
#define TEST_LOCK 1


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
    new_node->content_file = malloc(sizeof(unsigned char)+1);
    memset(new_node->content_file, 0, sizeof(unsigned char)+1);
    new_node->init_pointer_file = new_node->content_file;
    new_node->modified = -1;
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

    pthread_mutex_init(&storage_init->lock, NULL);

    printf("Storage create \n");

    return storage_init;
}

// revisione fatta, sembra corretto
int removeFile(struct node **file_remove, char ** rm_path, info_storage_t **storage, struct node ** current_file) {//First in Last out
    // se il file non e' aperto oppure est stato modificato almeno una volta, allora lo posso togliere dallo storage
    if ((*file_remove)->fdClient_id == -1) {// && (*file_remove)->modified != 0
        printf("Rimozione in corso ... : %s\n", (*file_remove)->pathname);

        struct node *tmp = (*file_remove);
        if ((*file_remove)->son == NULL) { // remove e aggiorno last file
            //printf("il nodo est LAST\n");
            if ((*file_remove)->father == NULL) { // unico nodo
                (*storage)->head = NULL;
                (*storage)->last = NULL;
                *current_file = NULL;
            } else {// e' LAST
                (*storage)->last = (*file_remove)->father;
                (*storage)->last->son = NULL;
                *current_file = (*storage)->last;
            }
        } else { // NB : rimuovo da LAST alla HEAD
            if ((*file_remove)->father == NULL) { // e' HEAD
                //printf("e' la HEAD\n");
                (*storage)->head = (*file_remove)->son;
                (*file_remove)->son->father = NULL;
                *current_file = NULL;
            } else { // e' in MIDDLE
                //printf("e' un MIDDLE\n");
                (*file_remove)->father = (*file_remove)->son;
                (*file_remove)->son = (*file_remove)->father;
                *current_file = (*file_remove)->father;
            }
        }
        *rm_path = realloc(*rm_path, strlen(*rm_path) + strlen((tmp)->pathname)+2);
        strncat(*rm_path, ",", 2);
        strncat(*rm_path, (tmp)->pathname, strlen((tmp)->pathname));
        // aggiorno ram, numero di file e il counter di file rimossi
        (*storage)->ram_dispo += (tmp)->file_sz;
        (*storage)->nfile_dispo += 1;
        free(tmp);
        return 0;
    }
    return -1;
}


int searchFileNode(char * pathname, info_storage_t ** storage, struct node ** file_found) {
    // search file a specific file in storage
    //printf("tot : %ld, dispo : %ld\n", storage->nfile_tot, storage->nfile_dispo);
    pthread_mutex_lock(&(*storage)->lock);
#if TEST_LOCK == 1
    printf("searchFileNode LOCK\n");
#endif
    long nb_file_check = (*storage)->nfile_tot-(*storage)->nfile_dispo;
    struct node * current;
    current = (*storage)->head;
    while (nb_file_check != 0) {
        printf("pathname : %s\n", (current)->pathname);
        if (strncmp((current)->pathname, pathname, strlen(pathname))==0){
            //printf("trovato !!\n");
            *file_found = current;
#if TEST_LOCK == 1
            printf("searchFileNode UNLOCK\n");
#endif
            pthread_mutex_unlock(&(*storage)->lock);
            return 0;
        } else {
            current = (current)->son;
        }
        nb_file_check -= 1;
    }
#if TEST_LOCK == 1
    printf("searchFileNode UNLOCK\n");
#endif
    pthread_mutex_unlock(&(*storage)->lock);
    return -1;
}

int insertCreateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path) {
    if ((*storage)->nfile_dispo <= 0) {
        int cnt = 0;
        struct node *current_file = (*storage)->last;
        struct node *current_ptr;
        while ((*storage)->nfile_dispo <= 0 && cnt < TIMER) {
            current_file = (*storage)->last;
            while (current_file != NULL && (*storage)->nfile_dispo <= 0) {
                pthread_mutex_lock(&(*storage)->lock);
                if (removeFile(&(current_file), &(*buf_rm_path), &(*storage), &current_ptr)) {
                    current_file = (current_file)->father;
                } else {
                    current_file = current_ptr;
                }
                pthread_mutex_unlock(&(*storage)->lock);
            }
            sleep((unsigned int) SLEEP_TIME);
            ++cnt;
        }
        if ((*storage)->nfile_dispo <= 0) {
            return -1;
        }
    }
    pthread_mutex_lock(&(*storage)->lock);
#if TEST_LOCK == 1
    printf("insertCreateFile 1 LOCK\n");
#endif
    if ((*storage)->head == NULL && (*storage)->last == NULL) { // ancora nessun file nello storage
        // se il primo file inserito ha una dimensione > della dim dello storage allora non lo inserisco
        (*storage)->head = *node_to_insert;
        (*storage)->last = *node_to_insert;
        (*storage)->nfile_dispo -= 1;
    } else {
        struct node *tmp_head = (*storage)->head;
        (*storage)->head = *node_to_insert;//aggiorno la head ptr dello storage col nuovo file
        (*node_to_insert)->son = tmp_head;
        (tmp_head)->father = *node_to_insert;
        // aggiorna disponibilita' dello spazio in memoria
        (*storage)->nfile_dispo -= 1;
    }
#if TEST_LOCK == 1
    printf("insertCreateFile 1.1 UNLOCK\n");
#endif
    pthread_mutex_unlock(&(*storage)->lock);
    return 0;

}

int UpdateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int fd_current, long size_buf, unsigned char * content) { // First in Last out
    // se file size > sz storage, rimuovo il file stesso
    if((*storage)->ram_tot - size_buf < 0 && (*node_to_insert)->modified ==-1) {
        struct node *notused;
        printf("UpdateFile 1 LOCK\n");
        pthread_mutex_lock(&(*storage)->lock);
        (*node_to_insert)->fdClient_id = -1;
        removeFile(&(*node_to_insert), &(*buf_rm_path), &(*storage), &notused);
        pthread_mutex_unlock(&(*storage)->lock);
        printf("UpdateFile 1 UNLOCK\n");
        return -1;
    }
    if((*storage)->ram_dispo - size_buf < 0) { // se ci sono piu' file nello storgae
        struct node *is_removable = (*storage)->last;
        struct node *current_file;
        int cnt = 0;
        while (((*storage)->ram_dispo - size_buf < 0) && cnt < TIMER) {
            is_removable = (*storage)->last;
            while (is_removable != NULL && ((*storage)->ram_dispo - size_buf < 0)) {
                printf("Current file storgae SIZE : %ld\ncurrent szie file : %ld\ncurrent file : %s\n",
                       (*storage)->ram_dispo, (is_removable)->file_sz, (is_removable)->pathname);
                printf("UpdateFile 2 UNLOCK\n");
                pthread_mutex_lock(&(*storage)->lock);
                if (removeFile(&(is_removable), &(*buf_rm_path), &(*storage), &(current_file)) == -1) {
                    // se file aperto da qualqun'altro , continuo a cercare nello storage
                    is_removable = (is_removable)->father; // check allora se un'altro file non est in stato aperto
                }
                is_removable = current_file;
                pthread_mutex_unlock(&(*storage)->lock);
                printf("UpdateFile 2 UNLOCK\n");
            }
            printf("Current file storgae fuori while SIZE : %ld\n", (*storage)->ram_dispo);
            sleep((unsigned int) SLEEP_TIME);
            ++cnt;
            printf("cnt : %d\n", cnt);
        }
        if ((*storage)->ram_dispo - size_buf < 0) {
            return -1;
        }
    }
    int cnt = 0;
    while (fd_current != (*node_to_insert)->fdClient_id && cnt < TIMER) {
        sleep((unsigned int) SLEEP_TIME);
        ++cnt;
    }
#if TEST_LOCK == 1
    printf("UpdateFile 4 LOCK\n");
#endif
    pthread_mutex_lock(&(*storage)->lock);
    if(fd_current != (*node_to_insert)->fdClient_id) {
#if TEST_LOCK == 1
        printf("UpdateFile 4.1 UNLOCK\n");
#endif
        pthread_mutex_unlock(&(*storage)->lock);
        return -1;
    } else {
        (*node_to_insert)->content_file = realloc((*node_to_insert)->content_file, size_buf+(*node_to_insert)->file_sz+1);
        (*node_to_insert)->init_pointer_file = (*node_to_insert)->content_file;
        memcpy((*node_to_insert)->content_file + (*node_to_insert)->file_sz, content, size_buf);
        (*node_to_insert)->file_sz += size_buf;
        (*storage)->ram_dispo -= size_buf;
        (*node_to_insert)->modified = 1;
#if TEST_LOCK == 1
        printf("UpdateFile 4.2 UNLOCK\n");
#endif
        pthread_mutex_unlock(&(*storage)->lock);
        return 0;
    }
}

int removeSpecificFile(char * pathname, info_storage_t ** storage, char ** pathname_removed) {
    // se voglio rimuovere uno specifico file
    struct node * find_file;
    struct node * current_file;
    if (searchFileNode(pathname, storage, &find_file) == 0) {
        printf("search file FOUND\n");
        printf("is open : %d\n", find_file->fdClient_id);
        if (find_file->fdClient_id != -1) {
            while (find_file->fdClient_id != 1) {
                printf("waiting...\n");
                sleep(1);
            }
        }
        removeFile(&find_file, pathname_removed, storage, &current_file);
        return 0;
    }
    return -1;
}


int printStorage(info_storage_t ** storage) {
    // Dice cosa contiene lo storage
#if TEST_LOCK == 1
    printf("printStorage LOCK\n");
#endif
    pthread_mutex_lock(&(*storage)->lock);
    printf("+++++++++++++++++++++++++++++\n");
    struct node * current = (*storage)->head;
    while(current != NULL) {
        printf("info : %s\nfd_id : %d\n", (current)->pathname, (current)->fdClient_id);
        current = (current)->son;
    }
    printf("+++++++++++++++++++++++++++++\n");
#if TEST_LOCK == 1
    printf("printStorage UNLOCK\n");
#endif
    pthread_mutex_unlock(&(*storage)->lock);
    return 0;
}


int freerStorage(info_storage_t ** storage) {
    // libero la memoria --------- FUNZIONE da DEFINIRE
#if TEST_LOCK == 1
    printf("freerStorage LOCK\n");
#endif
    pthread_mutex_lock(&(*storage)->lock);
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
#if TEST_LOCK == 1
    printf("freerStorage UNLOCK\n");
#endif
    pthread_mutex_unlock(&(*storage)->lock);
    return 0;
}