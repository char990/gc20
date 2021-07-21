#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <stdexcept>


#include "TcpServer.h"
#include "Epoll.h"

TcpServer::TcpServer(std::string name, int listenPort, int clientMax, ILowerLayer::LowerLayerType llType)
    : listenPort(listenPort),
      clientMax(clientMax),
      llType(llType),
      clientActive(0),
      clients(clientMax, nullptr),
      name(name)
{
    if ((eventFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw std::runtime_error("socket() failed");
    }
    SetNonblocking(eventFd);

    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listenPort);
    
    int reuse=1;
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
    {
        throw std::runtime_error("setsockopt(SO_REUSEPORT) failed");
    }
        
    if (bind(eventFd, (sockaddr *)&myserver, sizeof(myserver)) < 0)
    {
        throw std::runtime_error("bind() failed");
    }

    if (listen(eventFd, clientMax) < 0)
    {
        throw std::runtime_error("listen() failed");
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
    printf("Incomming...\n");
    int connfd = accept(eventFd, (sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        throw std::runtime_error(name+" accept() failed");
    }
    int x = -1;
    for (int i = 0; i < clientMax; i++)
    {
        if (clients[i] != nullptr)
        {
            if (clients[i]->GetFd() == -1)
            {
                delete clients[i];
                clients[i] = nullptr;
                clientActive--;
            }
        }
        if (x == -1)
        {
            if (clients[i] == nullptr)
            {
                x = i;
            }
        }
    }
    if (x == -1)
    {
        close(connfd);
        printf("Connections full, reject\n");
        return;
    }
    SetNonblocking(connfd);
    std::string clientname{name+"::TcpOperator" + std::to_string(x+1)};
    TcpOperator *client = new TcpOperator(clientname, connfd, llType);
    clients[x] = client;
    clientActive++;
    printf("Accept %s:[%d of %d]:%s\n", clientname.c_str(), clientActive, clientMax, inet_ntoa(clientaddr.sin_addr));
}

void TcpServer::EventsHandle(uint32_t events)
{
    if(events & EPOLLIN)
    {
        Accept();
    }
    else
    {
        UnknownEvents(name,events);
    }
}

void TcpServer::SetNonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        throw std::runtime_error("SetNonblocking()-fcntl(sock, F_GETFL) failed");
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        throw std::runtime_error("SetNonblocking()-fcntl(sock, F_SETFL) failed");
    }
}
