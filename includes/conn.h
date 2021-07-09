#if !defined(CONN_H)
#define CONN_H


int readn(long fd, void *buf, size_t size);
int writen(long fd, void *buf, size_t size);

#endif /* CONN_H */
