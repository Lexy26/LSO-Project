#ifndef API_H
#define API_H


#define O_CREATE 1
#define O_OPEN 0

// api_id = 1
int simple_opneConnection(const char *sockname, int msec, int maxtime);

// api_id = 2
int closeConnection(const char *sockname);

// api_id = 3
int openFile(const char *pathname, int flags);

// api_id = 4
int readFile(const char *pathname, void ** buf, size_t * size);

// api_id = 5
int readNFiles(int N, const char* dirname);

// api_id = 7
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

// api_id = 9
int closeFile(const char * pathname);


// ------ FUNZIONI Opzionali ------

int removeFile(const char * pathname);

int writeFile(const char *pathname, const char *dirname);

int lockFile(const char* pathname);

int unlockFile(const char* pathname);


#endif //API_H

