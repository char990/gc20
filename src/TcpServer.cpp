#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <stdexcept>

#include "TcpServer.h"
#include "Epoll.h"

#include "TsiSp003Lower.h"
#include "Web2AppLower.h"

Client::Client(std::string name, int fd, ILowerLayer::LowerLayerType llType)
    : name(name),
      clientFd(fd),
      llType(llType)
{
    events = EPOLLIN;
    Epoll::Instance().AddEvent(this, events);
    switch (llType)
    {
    case ILowerLayer::LowerLayerType::TSISP003LOWER:
        lowerLayer = new TsiSp003Lower();
        break;
    case ILowerLayer::LowerLayerType::WEB2APPLOWER:
        lowerLayer = new Web2AppLower();
        break;
    }
}

Client::~Client()
{
    switch (llType)
    {
    case ILowerLayer::LowerLayerType::TSISP003LOWER:
        delete (TsiSp003Lower *)lowerLayer;
        break;
    case ILowerLayer::LowerLayerType::WEB2APPLOWER:
        delete (Web2AppLower *)lowerLayer;
        break;
    }
    Release();
}

void Client::Release()
{
    if (clientFd > 0)
    {
        Epoll::Instance().DeleteEvent(this, events);
        close(clientFd);
        clientFd = -1;
    }
}

void Client::InEvent()
{
    lowerLayer->Rx();
    lowerLayer->Tx();

    uint8_t buf[65536];
    int n = read(clientFd, buf, 65536);
    if (n <= 0)
    {
        printf("[%s] disconnected\n", name.c_str());
        Release();
    }
    else
    {
        printf("[%s]%d bytes\n", name.c_str(), n);
    }
}

void Client::OutEvent()
{
}

void Client::Error(uint32_t events)
{
}

int Client::GetFd()
{
    return clientFd;
}

TcpServer::TcpServer(int listenPort, int clientMax, ILowerLayer::LowerLayerType llType)
    : listenPort(listenPort),
      clientMax(clientMax),
      llType(llType),
      clientActive(0),
      clients(clientMax, nullptr)
{
    if ((serverFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw std::runtime_error("socket() failed");
    }
    SetNonblocking(serverFd);

    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listenPort);

    if (bind(serverFd, (sockaddr *)&myserver, sizeof(myserver)) < 0)
    {
        throw std::runtime_error("bind() failed");
    }

    if (listen(serverFd, clientMax) < 0)
    {
        throw std::runtime_error("listen() failed");
    }
    events = EPOLLIN | EPOLLET;
    Epoll::Instance().AddEvent(this, events);
}

TcpServer::~TcpServer()
{
    Epoll::Instance().DeleteEvent(this, events);
    close(serverFd);
}

void TcpServer::InEvent()
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    printf("Incomming...\n");
    int connfd = accept(serverFd, (sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        throw std::runtime_error("accept() failed");
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
        return;
    }
    SetNonblocking(connfd);
    Client *client = new Client("CON" + std::to_string(x), connfd, llType);
    clients[x] = client;
    clientActive++;
    printf("Connection accepted at %d :[%d of %d]%s\n", x, clientActive, clientMax, inet_ntoa(clientaddr.sin_addr));
}

void TcpServer::OutEvent()
{
}

void TcpServer::Error(uint32_t events)
{
}

int TcpServer::GetFd()
{
    return serverFd;
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
