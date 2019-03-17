//warp.h
#ifndef _WARP_H
#define _WARP_H

#include <arpa/inet.h>
#include <unistd.h>

#define   UNBLOCK

//close
int Close(int fd);
//accept
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
//read
ssize_t Read(int fd, void *buf, size_t count);
//read unblock
ssize_t Read_unblock(int fd, void *buf, size_t count);
//write
ssize_t Write(int fd, const void *buf, size_t count);

#endif //_WARP_H