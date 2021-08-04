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
#include<fcntl.h>

#include "wrap.h"

#define MAXLINE 8192
#define SERV_PORT 9980


struct s_info{
    struct sockaddr_in cliaddr;
    int connfd;
};

void *do_work(void *arg){
    int n,i;
    struct s_info *ts=(struct s_info*)arg;
    char buf[MAXLINE];
    char str[INET_ADDRSTRLEN]; // #define INET_ADDRSTRLEN 16

    while(1){
        n=Read(ts->connfd,buf,MAXLINE); //读取客户端
        if(n==0){
            printf("the client %d close...\n",ts->connfd);//跳出循环，关闭cfd
            break;
        }
        printf("received from %s at PORT %d\n",inet_ntop(AF_INET,&(*ts).cliaddr.sin_addr,str,sizeof(str)),ntohs((*ts).cliaddr.sin_port));
        //打印客户端信息（IP/PORT）

        for(i=0;i<n;i++){
            buf[i]=toupper(buf[i]); //from 小 to 大
        }

        Write(STDOUT_FILENO,buf,n);//写出至屏幕

        Write(ts->connfd,buf,n);//回写给客户端

    }

    Close(ts->connfd);
    return (void *)0;
}

int main(void){
    
    struct sockaddr_in serv_addr,clit_addr;  //本机地址
    socklen_t clit_addr_len;
    int listenfd, connfd,ret;
    pthread_t tid;

    struct s_info ts[256];   //创建结构体数组
    int i=0;

    listenfd = Socket(AF_INET,SOCK_STREAM,0);  //创建套接字

    bzero(&serv_addr,sizeof(serv_addr));//clean struct zero
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);//指定本地任意IP
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);//指定端口号

    Bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)); //绑定端口

    Listen(listenfd,128);  //监听,设置同一时刻链接服务器上限数

    printf("accepting client connect...\n");

    while(1){
        clit_addr_len=sizeof(clit_addr);
        connfd=Accept(listenfd,(struct sockaddr *)&clit_addr,&clit_addr_len);
        //阻塞监听客户端链接请求
        ts[i].cliaddr=clit_addr;
        ts[i].connfd=connfd;

        pthread_create(&tid,NULL,do_work,(void*)&ts[i]);
        pthread_detach(tid);//子线程分离，防止僵线程产生
        i++;
    }

    return 0;
}