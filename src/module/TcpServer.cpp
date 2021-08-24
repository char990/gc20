#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <module/MyDbg.h>

#include <module/TcpServer.h>
#include <module/Epoll.h>

TcpServer::TcpServer(int listenPort, ObjectPool<OprTcp> & oPool, TimerEvent * tmr)
    : listenPort(listenPort),
      oPool(oPool),
      tmrEvt(tmr)
{
    name = "Tcp port:"+std::to_string(listenPort);
    if ((eventFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        MyThrow("%s:socket() failed", name.c_str());
    }
    SetNonblocking(eventFd);

    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listenPort);

    int reuse=1;
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        MyThrow("%s:setsockopt(SO_REUSEADDR) failed", name.c_str());
    }
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        MyThrow("%s:setsockopt(SO_REUSEPORT) failed", name.c_str());
    }
        
    if (bind(eventFd, (sockaddr *)&myserver, sizeof(myserver)) < 0)
    {
        MyThrow("%s:bind() failed", name.c_str());
    }

    if (listen(eventFd, oPool.Size()) < 0)
    {
        MyThrow("%s:listen() failed", name.c_str());
    }
    events = EPOLLIN | EPOLLET;
    Epoll::Instance().AddEvent(this, events);
}

TcpServer::~TcpServer()
{
    Epoll::Instance().DeleteEvent(this, events);
    close(eventFd);
}

void TcpServer::EventsHandle(uint32_t events)
{
    if(events & EPOLLIN)
    {
        Accept();
    }
    else
    {
        UnknownEvents(name, events);
    }
}

void TcpServer::Accept()
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    printf("%s:Incomming...\n",name.c_str());
    int connfd = accept(eventFd, (sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        MyThrow("%s:accept() failed", name.c_str());
    }
    OprTcp * tcpOperator = oPool.Pop();
    if(tcpOperator==nullptr)
    {
        close(connfd);
        printf("%s:Connections full, reject\n",name.c_str());
        return;
    }
    SetNonblocking(connfd);
    tcpOperator->SetServer(this);
    tcpOperator->Setup(connfd,tmrEvt);
    printf("%s:Accept %s:[%d of %d]:%s\n",
        name.c_str(), inet_ntoa(clientaddr.sin_addr), oPool.Cnt(), oPool.Size(), tcpOperator->Name().c_str());
}

void TcpServer::Release(OprTcp * tcpOperator)
{
    close(tcpOperator->GetFd());
    oPool.Push(tcpOperator);
    printf("%s:tcpOperator released:[%d of %d]:%s\n",
        name.c_str(), oPool.Cnt(), oPool.Size(), tcpOperator->Name().c_str());
}

void TcpServer::SetNonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        MyThrow("%s:SetNonblocking()-fcntl(sock, F_GETFL) failed", name.c_str());
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        MyThrow("%s:SetNonblocking()-fcntl(sock, F_SETFL) failed", name.c_str());
    }
}
