
#ifndef PROGETTO_LSO_UTIL_SERVER_H
#define PROGETTO_LSO_UTIL_SERVER_H


// come e' strutturato il messaggio di arrivo per il server
typedef struct {
    long api_id;
    char * sms_info;
    unsigned char * sms_content;
    int fd_client_id;
}sms_arg;

void threadF(sms_arg * smsArg);

#endif //PROGETTO_LSO_UTIL_SERVER_H
