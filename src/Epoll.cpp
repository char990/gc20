#include <cstdio>
#include <unistd.h>
#include <stdexcept>
#include "Epoll.h"

void Epoll::Init(int max)
{
    if(max<=0)
    {
        throw std::range_error("Epoll size must be greater than 0");
    }
    if(MAX>0)
    {
        throw std::runtime_error("Epoll Re-Init is not allowed");
    }
    MAX = max;
    cnt = 0;
    epollfd = epoll_create(MAX);
    if(epollfd<0)
    {
        throw std::runtime_error("Epoll create failed");
    }
}

Epoll::~Epoll()
{
    if(epollfd>0)close(epollfd);
}

void Epoll::AddEvent(IGcEvent * event, uint32_t events)
{
    if(cnt>=MAX)
    {
        throw std::overflow_error("Epoll overflow. Can't add event.");
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd,EPOLL_CTL_ADD, event->GetFd() ,&ev);
    cnt++;
}

void Epoll::DeleteEvent(IGcEvent * event, uint32_t events)
{
    if(cnt==0)
    {
        throw std::overflow_error("Epoll is empty. Can't delete event.");
    }
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd,EPOLL_CTL_DEL, event->GetFd() ,&ev);
    cnt--;
}

void Epoll::ModifyEvent(IGcEvent * event, uint32_t events)
{     
    struct epoll_event ev;
    ev.events = events;
    ev.data.ptr = event;
    epoll_ctl(epollfd,EPOLL_CTL_MOD, event->GetFd() ,&ev);
}

void Epoll::EventsHandle()
{
    epoll_event events[MAX];
    int num = epoll_wait(epollfd, events, MAX, -1);
    if(num == -1)
    {
        if(errno != EINTR)
        {
            throw std::runtime_error("epoll_wait() failed");
        }
    }
    for(int i = 0; i < num; i++)
    {
        IGcEvent * evt = (IGcEvent *)events[i].data.ptr;
        evt->EventsHandle(events[i].events);
    }
}

#if 0


#define IPADDRESS   "127.0.0.1"
#define PORT        8787
#define MAXSIZE     1024
#define LISTENQ     5
#define FDSIZE      1000
#define EPOLLEVENTS 100

listenfd = socket_bind(IPADDRESS,PORT);

struct epoll_event events[EPOLLEVENTS];

//创建一个描述符
epollfd = epoll_create(FDSIZE);

//添加监听描述符事件
add_event(epollfd,listenfd,EPOLLIN);

//循环等待
for ( ; ; ){
    //该函数返回已经准备好的描述符事件数目
    ret = epoll_wait(epollfd,events,EPOLLEVENTS,-1);
    //处理接收到的连接
    handle_events(epollfd,events,ret,listenfd,buf);
}
static void do_read(int epollfd,int fd,char *buf){
    int nread;
    nread = read(fd,buf,MAXSIZE);
    if (nread == -1)     {         
        perror("read error:");         
        close(fd); //记住close fd        
        delete_event(epollfd,fd,EPOLLIN); //删除监听 
    }
    else if (nread == 0)     {         
        fprintf(stderr,"client close.\n");
        close(fd); //记住close fd       
        delete_event(epollfd,fd,EPOLLIN); //删除监听 
    }     
    else {         
        printf("read message is : %s",buf);        
        //修改描述符对应的事件，由读改为写         
        modify_event(epollfd,fd,EPOLLOUT);     
    } 
}

//处理接收到的连接
static void handle_accpet(int epollfd,int listenfd){
     int clifd;     
     struct sockaddr_in cliaddr;     
     socklen_t  cliaddrlen;     
     clifd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen);     
     if (clifd == -1)         
     perror("accpet error:");     
     else {         
         printf("accept a new client: %s:%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);                       //添加一个客户描述符和事件         
         add_event(epollfd,clifd,EPOLLIN);     
     } 
}

//事件处理函数
static void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf)
{
     int i;
     int fd;
     //进行遍历;这里只要遍历已经准备好的io事件。num并不是当初epoll_create时的FDSIZE。
     for (i = 0;i < num;i++)
     {
         fd = events[i].data.fd;
        //根据描述符的类型和事件类型进行处理
         if ((fd == listenfd) &&(events[i].events & EPOLLIN))
            handle_accpet(epollfd,listenfd);
         else if (events[i].events & EPOLLIN)
            do_read(epollfd,fd,buf);
         else if (events[i].events & EPOLLOUT)
            do_write(epollfd,fd,buf);
     }
}


//写处理
static void do_write(int epollfd,int fd,char *buf) {     
    int nwrite;     
    nwrite = write(fd,buf,strlen(buf));     
    if (nwrite == -1){         
        perror("write error:");        
        close(fd);   //记住close fd       
        delete_event(epollfd,fd,EPOLLOUT);  //删除监听    
    }else{
        modify_event(epollfd,fd,EPOLLIN); 
    }    
    memset(buf,0,MAXSIZE); 
}
#endif
