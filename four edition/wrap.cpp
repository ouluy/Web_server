#include "wrap.h"
//#include<iostream>


void perr_exit(const char *str){
    perror(str);
    exit(1);
}

/*int Accept(int fd,struct sockaddr *sa,socklen_t *salenptr){
    int n;
again:
    if((n=accept(fd,sa,salenptr))<0){
        if((errno==ECANCELED)||(errno==EINTR)){
            goto again;
        }
        else perr_exit("accept error");
    }

return n;

}*/

int Bind(int fd,const struct sockaddr *sa,socklen_t salen){
    int n;
    if((n=bind(fd,sa,salen))<0){
        perr_exit("bind error");
    }
    return n;
}

int Connect(int fd,const struct sockaddr *sa,socklen_t salen){
    int n;
    if((n=connect(fd,sa,salen))<0){
        perr_exit("connect error");
    }
    return n;
}

int Listen(int fd,int backlog){
    int n;
    if((n=listen(fd,backlog))<0){
        perr_exit("listen error");
    }
    return n;
}


int Socket(int family,int type,int protocol){
    int n;
    if((n=socket(family,type,protocol))<0){
        perr_exit("socket error");
    }
    return n;
}

ssize_t Read(int fd,void *ptr,size_t nbytes){
    ssize_t n;
again:
    if((n=read(fd,ptr,nbytes))==-1){
        if(errno==EINTR){
            goto again;
        }
        else return -1;
    }
    return n;
}

ssize_t Write(int fd,const void *ptr,size_t nbytes){
    ssize_t n;
again:
    if((n=write(fd,ptr,nbytes))==-1){
        if(errno==EINTR){
            goto again;
        }
        else return -1;
    }
    return n;
}

int Close(int fd){
    int n;
    if((n=close(fd))==-1){
        perr_exit("close error");
    }
    return n;
}

ssize_t readn(int fd,void *vptr,size_t n){
    size_t nleft=n;
    ssize_t nread=0;
    ssize_t readSum=0;
    char *ptr=(char *)vptr;
    nleft=n;
    while(nleft>0){
        if((nread=read(fd,ptr,nleft))<0){
            if(errno==EINTR){
                nread=0;
            }
            else if(errno==EAGAIN){
                return readSum;
            }
            else{
                return -1;
            }
        }
        else if(nread==0){
            break;
        }
       // std::cout<<"readsum:"<<readSum<<std::endl;
        readSum+=nread;
        nleft-=nread;
        ptr+=nread;
    }
    return readSum;
}
/*
ssize_t readn(int fd, std::string &inBuffer)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                
                return readSum;
            }  
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (nread == 0)
            break;
        //printf("before inBuffer.size() = %d\n", inBuffer.size());
        //printf("nread = %d\n", nread);
        readSum += nread;
        //buff += nread;
        inBuffer += std::string(buff, buff + nread);
        //printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}*/

ssize_t Writen(int fd,const void *vptr,size_t n){
    size_t nleft;
    ssize_t nwritten;
    ssize_t writeSum=0;
    char *ptr;
    ptr=(char*)vptr;
    nleft=n;
    while(nleft>0){
        if((nwritten=write(fd,ptr,nleft))<=0){
            if(nwritten<0){
                if(errno==EINTR || errno==EAGAIN){
                    nwritten=0;
                }
                else{
                    return -1;
                }
            }
        }
        writeSum+=nwritten;
        nleft-=nwritten;
        ptr+=nwritten;
    }
    return n;
}


void handle_for_sigpipe(){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

int setSocketNonBlocking(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}



