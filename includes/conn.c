
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "conn.h"

/** Evita letture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la lettura da fd leggo EOF
 *   \retval size se termina con successo
 */
int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    ssize_t r;
    void *bufptr = buf;
    while(left>0) {
        if ((r=read((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;   // EOF
        left    -= r;
        bufptr  += r;
    }
    return (int) size;
}

/** Evita scritture parziali
 *
 *   \retval -1   errore (errno settato)
 *   \retval  0   se durante la scrittura la write ritorna 0
 *   \retval  1   se la scrittura termina con successo
 */
int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    ssize_t r;
    void *bufptr = buf;
    while(left>0) {
        if ((r=write((int)fd ,bufptr,left)) == -1) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return 0;
        left    -= r;
        bufptr  += r;
    }
    return 1;
}
