

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

typedef struct {
    int N_THREAD; // number of threads in server
    int N_FILE; // max number of possible file in storage
    int MEM_SIZE; //max size of memory storage in Mbytes
} config_t;

void configuration(int argc,char *argv[],config_t **cfg);


#endif //CONFIGURATION_H
