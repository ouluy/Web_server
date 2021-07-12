#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include "wrap.h"

#define SERV_PORT 9999

void sys_err(const char *str){
    perror(str);
    exit(1);
}

int main(int argc,char *argv[]){

        int cfd;
        char buf[BUFSIZ];
        int number=9;

        struct sockaddr_in serv_addr;
        serv_addr.sin_family=AF_INET;
        serv_addr.sin_port= htons(SERV_PORT);
        
        inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr.s_addr);
        
        cfd = socket(AF_INET,SOCK_STREAM,0);

        if(cfd==-1){
            sys_err("socket error");
        }
        
        int ret = Connect(cfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

        if(ret==-1){
            sys_err("connect error");
        }

        while(number--){
            Write(cfd,"hello",5);
            ret=Read(cfd,buf,sizeof(buf));
            Write(STDOUT_FILENO,buf,ret);
        }
        Close(cfd);

    return 0;
}