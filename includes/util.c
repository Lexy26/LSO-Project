/*             Send or Received MESSAGE CLIENT-SERVER              */

#define _POSIX_C_SOURCE  200112L // per strtok_r
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#include "util.h"
#include "conn.h"

// ------  GENERAL STRUCTURE OF MESSAGE  ------ //

/* Send from CLIENT to SERVER
 * message structure (Request):
 *                      API_ID     ARG1      ARG2          ARG3
 * - OpenFile        -> api_id + pathname +  flag
 * - AppendToFile    -> api_id + pathname + size_byte + content_file
 * - CloseFile       -> api_id + pathname
 * - CloseConnection -> api_id + sockname
 * - readFile        -> api_id + pathname
 * - readNFile       -> api_id +    N
 * */

/* Send from SERVER to CLIENT
 * message structure (Answer):
 *                         ARG1          ARG2        ARG3       ARG4
 * - OpenFile        -> -1 ko| 0   ok + file removed
 * - AppendToFile    -> -1 ko| 0,1 ok + file removed
 * - CloseFile       -> -1 ko| 0   ok
 * - CloseConnection -> -1 ko| 0   ok
 * - readFile        -> -1 ko| 0   ok + size_buf + content_buf
 * - readNFile       -> -1 ko| 0   ok + pathname + size_buf + content_buf
 * */
// -------------------------------------------- //


/*!
 * sendMsg_File_Content : send message info + content of file
 * @param fd_c   : client fd
 * @param api_id : request identifier (from client) / validation request or not (from server)
 * @param arg1   : pathname
 * @param arg2   : size of content
 * @param arg3   : content of file
 */
void sendMsg_File_Content(int fd_c, char *api_id, char *arg1, char *arg2, unsigned char *arg3) {
    sendMsg(fd_c, api_id, arg1, arg2);
    msg_t *sms_content;
    CHECK_EXIT_VAR("malloc sms_content", sms_content, malloc(sizeof(msg_t)), NULL)
    memset(sms_content, 0, sizeof(msg_t));
    sms_content->len = strtol(arg2, NULL, 10);
    CHECK_EXIT_VAR("calloc sms_content->str", sms_content->str, calloc(sms_content->len, sizeof(unsigned char)), NULL)
    memcpy(sms_content->str, arg3, sms_content->len);
    CHECK_EQ_EXIT("writen sendMsg_File_Content sms", writen(fd_c, sms_content->str, sms_content->len), -1)
    free(sms_content->str);
    free(sms_content);
}

/*!
 * sendMsg : send message info
 * @param fd_c   : client fd
 * @param api_id : request identifier (from client) / validation request or not (from server)
 * @param arg1   : pathname or sockname
 * @param arg2   : flag or size_buf
 */
void sendMsg(int fd_c, char api_id[3], char *arg1, char *arg2) {
    msg_t *sms_write;
    CHECK_EXIT_VAR("calloc sms_write", sms_write, malloc(sizeof(msg_t)), NULL)
    memset(sms_write, 0, sizeof(msg_t));
    sms_write->len = strlen(api_id);
    if(arg1 != NULL) {
        sms_write->len += strlen(arg1);
        if (arg2 != NULL) sms_write->len += strlen(",");
    }
    if (arg2 != NULL) {
        sms_write->len += strlen(arg2);
    }
    CHECK_EXIT_VAR("calloc sms_write->str", sms_write->str, calloc(sms_write->len+2, sizeof(unsigned char)), NULL)
    strncpy((char *) sms_write->str, api_id, 3);
    if ( arg1 != NULL) {
        strncat((char *)sms_write->str, arg1, strlen(arg1));
        if (arg2 != NULL) strncat((char *)sms_write->str, ",", 2);
    }
    if (arg2 != NULL) {
        strncat((char *)sms_write->str, arg2, strlen(arg2));
    }
    CHECK_EQ_EXIT("writen sendMsg size", writen(fd_c, &sms_write->len, sizeof(size_t)), -1)
    CHECK_EQ_EXIT("writen sendMsg sms", writen(fd_c, sms_write->str, sms_write->len), -1)
    free(sms_write->str);
    free(sms_write);
}


/*!
 * receivedMsg_File_Content : received message info + file content
 * @param pathname    : pathname that identify the file
 * @param sms_content : content of file
 * @param size_buf    : size of content to read
 * @param check       : see if file to read exist (1), there is no file (-1) or no need to read other file (0)
 * @param fd_c
 */
void receivedMsg_File_Content(char ** pathname, unsigned char** sms_content, size_t *size_buf,int * check, int fd_c){
    unsigned char * sms_info;
    recievedMsg(&sms_info, fd_c);
    char * tmp;
    char * token = strtok_r((char *)sms_info, ",", &tmp); // token = check
    if (strncmp(token, "1", 1)==0) { // there is a file to read
        msg_t * sms;
        CHECK_EXIT_VAR("malloc sms", sms, malloc(sizeof(msg_t)), NULL)
        memset(sms, 0, sizeof(msg_t));
        token = strtok_r(NULL, ",", &tmp);
        *pathname = token;
        token = strtok_r(NULL, ",", &tmp);
        int size_byte = strtol(token, NULL, 10);
        *size_buf = size_byte;
        CHECK_EXIT_VAR("calloc sms->str", sms->str, calloc(size_byte, sizeof(unsigned char)), NULL)
        sms->len = size_byte;
        CHECK_EQ_EXIT("readn receivedMsg_File_Content sms", readn(fd_c, sms->str, sms->len), -1)
        *sms_content = sms->str;
        *check = 1;
        free(sms);
    } else if (strncmp(token, "0", 1)==0){ // there are no more files to read
        printf("finito tutto in bellezza \n");
        *size_buf = 0;
        *sms_content = NULL;
        *check = 0;
        *pathname = NULL;
    } else { // error, something went wrong
        printf("errore \n");
        *size_buf = 0;
        *sms_content = NULL;
        *check = -1;
        *pathname = NULL;
    }
    //free(sms_info);

}

/*!
 * recievedMsg: received message info
 * @param sms_info : useful information for client or server
 * @param fd_c
 */
void recievedMsg(unsigned char ** sms_info, int fd_c) {
    msg_t *sms_read;
    CHECK_EXIT_VAR("malloc sms_read", sms_read, malloc(sizeof(msg_t)), NULL)
    memset(sms_read, 0, sizeof(msg_t));
    CHECK_EQ_EXIT("readn recievedMsg sms", readn(fd_c, &sms_read->len, sizeof(size_t)), -1)
    CHECK_EXIT_VAR("calloc sms_read->str", sms_read->str, calloc(1, sms_read->len+1), NULL)
    CHECK_EQ_EXIT("readn recievedMsg sms", readn(fd_c, sms_read->str, sms_read->len), -1)
    *sms_info = sms_read->str;
    free(sms_read);
}


