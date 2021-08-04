#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<strings.h>
#include<signal.h>
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
    
    struct sockaddr_in serv_addr,clit_addr;
    socklen_t clit_addr_len;

    int client[FD_SETSIZE];//自定义数组client 防止遍历1024个文件描述符 FD_SETSIZE默认为1024

    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(SERV_PORT);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    int lfd=0, cfd=0,ret;
    char buf[BUFSIZ],client_IP[1024];

    lfd=Socket(AF_INET,SOCK_STREAM,0);

    Bind(lfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

    Listen(lfd,128);

    fd_set rset,allset;  //定义 读集合，备份集合allset
    int maxfd=0,n,i,j,maxi;   
    maxfd=lfd;         //最大文件描述符

    maxi=-1;
    for(i=0;i<FD_SETSIZE;i++){
        client[i]=-1;
    }

    FD_ZERO(&allset);    //清空 监听集合

    FD_SET(lfd,&allset); //将待监听fd添加到监听集合中

    while(1){
        rset = allset;   //备份
        ret =select(maxfd+1,&rset,NULL,NULL,NULL);//使用select监听
        if(ret<0){
            perr_exit("select error");
        }

        if(FD_ISSET(lfd,&rset)){           //lfd 满足监听的 读事件
            clit_addr_len=sizeof(clit_addr);

            cfd=Accept(lfd,(struct sockaddr *)&clit_addr,&clit_addr_len);
            //建立链接....不会阻塞

            printf("client ip:%s port:%d\n",inet_ntop(AF_INET,&clit_addr.sin_addr.s_addr,client_IP,sizeof(client_IP)),ntohs(clit_addr.sin_port));

           
            for(i=0;i<FD_SETSIZE;i++){   
                if(client[i]<0){     //找client[]中没有使用的位置
                    client[i]=cfd;   //保存accept返回的文件描述符到client[]里
                    break;
                }
            }
            if(i==FD_SETSIZE){     //达到select能监控的文件个数上限1024
                fputs("too many clients\n",stderr);
                exit(0);
            }
            FD_SET(cfd,&allset); //将新产生的fd，添加到监听集合中，监听数据读事件
            
            if(cfd>maxfd){
                maxfd=cfd;     //select第一个参数需求
            }
            
            if(i>maxi){    //保证maxi存的总是client[]最后一个元素下标
                maxi=i; 
            }
            if(--ret==0){
                continue;
            }
            

            
            //if(ret==1) continue;//说明select 只返回一个，并且是lfd，后续执行无须执行
        }


        for(i=0;i<=maxi;i++){   //处理满足读事件的fd
            int sockfd=client[i];
            if(sockfd<0){continue;}
            if(FD_ISSET(sockfd,&rset)){  //找到满足读事件的那个fd
                n = Read(sockfd,buf,sizeof(buf));

                if(n==0){       //检测到客户端已经关闭链接
                    Close(sockfd);
                    FD_CLR(sockfd,&allset);     //将关闭的fd，移除出监听集合
                    client[i]=-1;
                }
                else if(n==-1){
                    perr_exit("read error");
                }
                else if(n>0){ 
                    for(j=0;j<n;j++){
                        buf[j]=toupper(buf[j]);
                    }
  
                    Write(sockfd,buf,n); 
                    Write(STDOUT_FILENO,buf,n);
                }
                if(--ret==0){  //退出for，还在while
                    break;
                }
            }
        }

    }

    Close(lfd);

    return 0;
}
