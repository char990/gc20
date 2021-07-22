#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <stdexcept>

#include "TcpServer.h"
#include "Epoll.h"

TcpServer::TcpServer(int listenPort, ObjectPool<TcpOperator> & oPool)
    : listenPort(listenPort),
      oPool(oPool)
{
    name = "Tcp port:"+std::to_string(listenPort);
    if ((eventFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw std::runtime_error(name+":socket() failed");
    }
    SetNonblocking(eventFd);

    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listenPort);

    int reuse=1;
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        throw std::runtime_error(name+":setsockopt(SO_REUSEADDR) failed");
    }
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        throw std::runtime_error(name+":setsockopt(SO_REUSEPORT) failed");
    }
        
    if (bind(eventFd, (sockaddr *)&myserver, sizeof(myserver)) < 0)
    {
        throw std::runtime_error(name+":bind() failed");
    }

    if (listen(eventFd, oPool.Size()) < 0)
    {
        throw std::runtime_error(name+":listen() failed");
    }
    events = EPOLLIN | EPOLLET;
    Epoll::Instance().AddEvent(this, events);

}

TcpServer::~TcpServer()
{
    Epoll::Instance().DeleteEvent(this, events);
    close(eventFd);
}

void TcpServer::Accept()
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    printf("%s:Incomming...\n",name.c_str());
    int connfd = accept(eventFd, (sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        throw std::runtime_error(name+":accept() failed");
    }
    TcpOperator * tcpOperator = oPool.Pop();
    if(tcpOperator==nullptr)
    {
        close(connfd);
        printf("%s:Connections full, reject\n",name.c_str());
        return;
    }
    SetNonblocking(connfd);
    tcpOperator->Setup(connfd);
    printf("%s:Accept %s:[%d of %d]\n",name.c_str(), inet_ntoa(clientaddr.sin_addr), oPool.Cnt(), oPool.Size());
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

void TcpServer::SetNonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        throw std::runtime_error(name+":SetNonblocking()-fcntl(sock, F_GETFL) failed");
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        throw std::runtime_error(name+":SetNonblocking()-fcntl(sock, F_SETFL) failed");
    }
}
