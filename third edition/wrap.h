#ifndef _WRAP_H_
#define _WRAP_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<signal.h>
#include <fcntl.h>

void perr_exit(const char *s);
//int Accept(int fd,struct sockaddr *sa,socklen_t *salenptr);
int Bind(int fd,const struct sockaddr *sa,socklen_t salen);
int Connect(int fd,const struct sockaddr *sa,socklen_t salen);
int Listen(int fd,int backlog);
int Socket(int family,int type,int protocol);
ssize_t Read(int fd,void *ptr,size_t nbytes);
ssize_t Write(int fd,const void *ptr,size_t nbytes);
int Close(int fd);
ssize_t readn(int fd,void *vptr,size_t n);
ssize_t Writen(int fd,const void *vptr,size_t n);
void handle_for_sigpipe();
int setSocketNonBlocking(int fd);

#endif