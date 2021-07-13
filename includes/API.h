#ifndef API_H
#define API_H


#define O_CREATE 1
#define O_OPEN 0

struct timespec;


int openConnection(const char *sockname, int msec, const struct timespec abstime);

// api_id = 1
int closeConnection(const char *sockname);

// api_id = 2
int openFile(const char *pathname, int flags);

// api_id = 3
int readFile(const char *pathname, void ** buf, size_t * size);

// api_id = 4
int readNFiles(int N, const char* dirname);

// api_id = 5
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

// api_id = 6
int closeFile(const char * pathname);


// ------ FUNZIONI Opzionali ------

int removeFile(const char * pathname);

int writeFile(const char *pathname, const char *dirname);

int lockFile(const char* pathname);

int unlockFile(const char* pathname);


#endif //API_H

