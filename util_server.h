
#ifndef UTIL_SERVER_H
#define UTIL_SERVER_H

#include "file_storage.h"

// come e' strutturato il messaggio di arrivo per il server
typedef struct {
    long api_id;
    long size_buf;
    char * sms_info;
    unsigned char * sms_content;
    int fd_client_id;
    int pipe_fd;
}sms_arg;

void threadF(sms_arg * smsArg, info_storage_t ** storage);

#endif //UTIL_SERVER_H
