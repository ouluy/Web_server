#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<signal.h>
#include<unistd.h>
#include<errno.h>
#include<ctype.h>
#include<sys/wait.h>
#include<pthread.h>
#include<sys/socket.h>
#include<arpa/inet.h>

#include "wrap.h"

#define SERV_PORT 9999


int main(int argc,char *argv[]){


    signal(SIGCHLD,SIG_IGN);

    struct sockaddr_in serv_addr,clit_addr;  //本机地址
    socklen_t clit_addr_len;

    bzero(&serv_addr,sizeof(serv_addr));//clean struct zero

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int lfd, cfd,ret;
    char buf[BUFSIZ],client_IP[1024];
    pid_t pid;

    lfd = Socket(AF_INET,SOCK_STREAM,0);  //创建套接字

    Bind(lfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)); //绑定端口

    Listen(lfd,128);  //监听

    clit_addr_len=sizeof(clit_addr); 

    while(1){
        cfd = Accept(lfd,(struct sockaddr *)&clit_addr,&clit_addr_len);
        
        pid = fork();

        if(pid<0){
            perr_exit("fork error");
        }
        else if(pid==0){ //子进程
            Close(lfd);
            while(1){
                 ret = Read(cfd,buf,sizeof(buf));
                 if(ret==-1){
                     perr_exit("read error");
                 }
                 else if(ret==0){
                     break;
                 }
                  Write(STDOUT_FILENO,buf,ret);

                  for(int i=0;i<ret;i++){
                     buf[i]=toupper(buf[i]);
                  }
                 Write(cfd,buf,ret);
            }
            //Close(cfd);
            exit(0);
        }
        else{ //父进程
            
            Close(cfd);
        }

    }

    Close(cfd);
    Close(lfd);

    return 0;
}