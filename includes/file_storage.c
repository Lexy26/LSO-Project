/* ------------------ FILE STORAGE SERVER -------------------- */
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "util.h"
#include "file_storage.h"

#define TEST_LOCK 0


/*!
 * createFileNode : initialise file node structure
 * @param name : pathname
 * @param fd : fd client who create this file
 * @return : file node
 */
struct node *createFileNode(char * name, int fd) {
    struct node *new_node;
    CHECK_EXIT_VAR("malloc new node", new_node, malloc(sizeof(struct node)), NULL)
    memset(new_node, 0, sizeof(struct node));
    new_node->file_sz = 0; // in byte
    new_node->fdClient_id = fd;
    new_node->son = NULL;
    new_node->father = NULL;
    new_node->pathname = name;
    new_node->content_file = malloc(sizeof(unsigned char)+1);
    memset(new_node->content_file, 0, sizeof(unsigned char)+1);
    new_node->modified = -1;
    return new_node;
}

/*!
 * createStorage : create and initialise the storage
 * @param ram : memory size of storage given by the configuration file
 * @param nfile : file number given by the configuration file
 * @return      : storage struct initialised
 */
info_storage_t *createStorage(long ram, long nfile) {
    info_storage_t *storage_init;
    CHECK_EXIT_VAR("calloc storage init", storage_init, calloc(1, sizeof(info_storage_t)), NULL)

    storage_init->head = NULL;
    storage_init->last = NULL;

    storage_init->nfile_dispo = nfile;
    storage_init->nfile_tot = nfile;

    storage_init->ram_dispo = ram;
    storage_init->ram_tot = ram;
    CHECK_NEQ_EXIT("pthread_mutex_init", pthread_mutex_init(&storage_init->lock, NULL), 0)
    fprintf(stderr, "Storage create \n");
    return storage_init;
}

/*!
 * removeFile           : removes the file from storage and updates the storage
 * @param file_remove   : file that should be removed
 * @param rm_path       : if file removed, save path of removed file in this variable
 * @param storage       : if file removed, update storage
 * @param current_file  : if file removed, updates the pointer to the storage
 * @param nfile_removed : if file removed, increase the variable by one
 * @return              : file removed (0) | file not removed (-1)
 */
int removeFile(struct node **file_remove, char ** rm_path, info_storage_t **storage, struct node ** current_file, int * nfile_removed) {
    if ((*file_remove)->fdClient_id == -1) {
        fprintf(stderr, "Rimozione in corso ... : %s\n", (*file_remove)->pathname);
        (*nfile_removed) += 1;
        struct node *tmp = (*file_remove);
        if ((*file_remove)->son == NULL) { // remove e aggiorno last file
            // only one node remains
            if ((*file_remove)->father == NULL) {
                (*storage)->head = NULL;
                (*storage)->last = NULL;
                *current_file = NULL;
            } else {
                // is LAST node
                (*storage)->last = (*file_remove)->father;
                (*storage)->last->son = NULL;
                *current_file = (*storage)->last;
            }
        } else { // NB : remove from LAST to HEAD
            if ((*file_remove)->father == NULL) {
                // is HEAD node
                (*storage)->head = (*file_remove)->son;
                (*file_remove)->son->father = NULL;
                *current_file = NULL;
            } else {
                // is in the MIDDLE
                (*file_remove)->father = (*file_remove)->son;
                (*file_remove)->son = (*file_remove)->father;
                *current_file = (*file_remove)->father;
            }
        }
        CHECK_EXIT_VAR("realloc rm_path", *rm_path, realloc(*rm_path, strlen(*rm_path) + strlen((tmp)->pathname)+2), NULL)
        strncat(*rm_path, ",", 2);
        strncat(*rm_path, (tmp)->pathname, strlen((tmp)->pathname));
        // update storage parameters
        (*storage)->ram_dispo += (tmp)->file_sz;
        (*storage)->nfile_dispo += 1;
        free(tmp->content_file);
        free(tmp);
        return 0;
    }
    return -1;
}

/*!
 * searchFileNode    : search file structure that i need
 * @param pathname   : path of file structure searched
 * @param storage    : search file from the storage
 * @param file_found : save file found in this variable
 * @return           : file found (0) | not found (-1)
 */
int searchFileNode(char * pathname, info_storage_t ** storage, struct node ** file_found) {
    LOCK(&(*storage)->lock, -1)
    long nb_file_check = (*storage)->nfile_tot-(*storage)->nfile_dispo;
    struct node * current;
    current = (*storage)->head;
    while (nb_file_check != 0) {
        // file found
        if (strncmp((current)->pathname, pathname, strlen(pathname))==0){
            *file_found = current;
            UNLOCK(&(*storage)->lock, -1)
            return 0;
        } else {
            current = (current)->son; // next
        }
        nb_file_check -= 1;
    }
    UNLOCK(&(*storage)->lock, -1)
    return -1;
}

/*!
 * insertCreateFile      : insert newly created file node, without content
 * @param node_to_insert : node to insert in storage
 * @param storage        : storage
 * @param buf_rm_path    : var where save removed file
 * @param nfile_removed  : if file removed, increase the variable by one
 * @return               : insert (0) | not insert (-1)
 */
int insertCreateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int * nfile_removed) {
    if ((*storage)->nfile_dispo <= 0) {
        int cnt = 0;
        struct node *current_file = (*storage)->last;
        struct node *current_ptr;
        // if i have reached the maximum number of files on the storage
        // loop to remove files from the storage until enough space is freed up to insert the new file
        while ((*storage)->nfile_dispo <= 0 && cnt < TIMER) {
            current_file = (*storage)->last;
            while (current_file != NULL && (*storage)->nfile_dispo <= 0) {
                LOCK(&(*storage)->lock, -1)
                if (removeFile(&(current_file), &(*buf_rm_path), &(*storage), &current_ptr, &(*nfile_removed))) {
                    current_file = (current_file)->father;
                } else {
                    current_file = current_ptr;
                }
                UNLOCK(&(*storage)->lock, -1)
            }
            sleep((unsigned int) SLEEP_TIME);
            ++cnt;
        }
        // if space is still insufficient
        if ((*storage)->nfile_dispo <= 0) {
            return -1;
        }
    }
    // if no problem, insert new file in storage
    LOCK(&(*storage)->lock, -1)
    if ((*storage)->head == NULL && (*storage)->last == NULL) {
        (*storage)->head = *node_to_insert;
        (*storage)->last = *node_to_insert;
    } else {
        struct node *tmp_head = (*storage)->head;
        (*storage)->head = *node_to_insert;
        (*node_to_insert)->son = tmp_head;
        (tmp_head)->father = *node_to_insert;
    }
    (*storage)->nfile_dispo -= 1;
    UNLOCK(&(*storage)->lock, -1)
    return 0;

}

/*!
 * UpdateFile            : add the content to the file already created
 * @param node_to_insert : node to which i need to update the content
 * @param storage        : global storage
 * @param buf_rm_path    : var where save removed file
 * @param fd_current     : is the client interested in updating the file
 * @param size_buf       : size of content
 * @param content        : is the content i have to add
 * @param nfile_removed  : if file removed, increase the variable by one
 * @return               : update done (0) | update error (-1)
 */
int UpdateFile(struct node ** node_to_insert, info_storage_t **storage, char ** buf_rm_path, int fd_current, long size_buf, unsigned char * content, int * nfile_removed) { // First in Last out
    // if file size is bigger than the global size of storage, delete file itself
    if((*storage)->ram_tot - size_buf < 0 && (*node_to_insert)->modified ==-1) {
        struct node *notused;
        LOCK(&(*storage)->lock, -1)
        (*node_to_insert)->fdClient_id = -1;
        removeFile(&(*node_to_insert), &(*buf_rm_path), &(*storage), &notused, &(*nfile_removed));
        UNLOCK(&(*storage)->lock, -1)
        return -1;
    }
    // if insufficient memory, remove as many files as necessary
    if((*storage)->ram_dispo - size_buf < 0) {
        struct node *is_removable = (*storage)->last;
        struct node *current_file;
        // 'cnt' is necessary to prevent the cycle from going to infinity
        int cnt = 0;
        while (((*storage)->ram_dispo - size_buf < 0) && cnt < TIMER) {
            is_removable = (*storage)->last;
            while (is_removable != NULL && ((*storage)->ram_dispo - size_buf < 0)) {
                LOCK(&(*storage)->lock, -1)
                if (removeFile(&(is_removable), &(*buf_rm_path), &(*storage), &(current_file), &(*nfile_removed)) == -1) {
                    // if the current file is open from another client, check next file
                    is_removable = (is_removable)->father;
                }
                is_removable = current_file;
                UNLOCK(&(*storage)->lock, -1)
            }
            sleep((unsigned int) SLEEP_TIME);
            ++cnt;
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
    LOCK(&(*storage)->lock, -1)
    if(fd_current != (*node_to_insert)->fdClient_id) {
        UNLOCK(&(*storage)->lock, -1)
        return -1;
    } else {
        CHECK_EXIT_VAR("realloc content file", (*node_to_insert)->content_file, realloc((*node_to_insert)->content_file, size_buf+(*node_to_insert)->file_sz+1), NULL)
        memcpy((*node_to_insert)->content_file + (*node_to_insert)->file_sz, content, size_buf);
        (*node_to_insert)->file_sz += size_buf;
        (*storage)->ram_dispo -= size_buf;
        (*node_to_insert)->modified = 1;
        UNLOCK(&(*storage)->lock, -1)
        return 0;
    }
}

// print all pathname of file in the storage and size
void printStorage(info_storage_t ** storage, FILE * logfile) {
    LOCK(&(*storage)->lock, )
    fprintf(logfile, "************************************************************\n");
    fprintf(logfile, "******                    STORAGE                     ******\n");
    fprintf(logfile, "************************************************************\n\n");
    struct node * current = (*storage)->head;
    while(current != NULL) {
        fprintf(logfile, "Info : %s\nSize : %ld\n", (current)->pathname, (current)->file_sz);
        current = (current)->son;
    }
    fprintf(logfile, "\n************************************************************\n");
    UNLOCK(&(*storage)->lock, )
}

// clears the entire storage
void freerStorage(info_storage_t ** storage) {
    LOCK(&(*storage)->lock, )
    long nb_file_check = (*storage)->nfile_tot-(*storage)->nfile_dispo;
    struct node * current = (*storage)->head;
    struct node * tmp;
    while (current != NULL) {
        tmp = current;
        current = current->son;
        free(tmp->content_file);
        free(tmp);
        fprintf(stderr, "file free : %ld\n", nb_file_check);
        nb_file_check -= 1;
    }
    UNLOCK(&(*storage)->lock, )
    free(*storage);
}