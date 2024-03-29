#include <cstring>
#include <errno.h>
#include <fcntl.h>

#include <module/MyDbg.h>

#include <module/TcpServer.h>
#include <module/Epoll.h>
#include <module/ObjectPool.h>
#include <uci/DbHelper.h>

#include <module/Utils.h>

using namespace Utils;
using namespace std;

TcpServer::TcpServer(int listenPort, TcpSvrType serverType, int poolsize, TimerEvent *tmr)
    : listenPort(listenPort),
      serverType(serverType),
      poolsize(poolsize),
      tmrEvt(tmr)
{
    name = TcpSvrTypeName(serverType) + " server:" + to_string(listenPort);

    objPool = new ObjectPool<OprTcp>(poolsize);
    auto ipool = objPool->Pool();
    for (int i = 0; i < ipool.size(); i++)
    {
        ipool[i]->Init(i, serverType);
    }
    eventFd = -1;
    Open();
}

TcpServer::~TcpServer()
{
    Close();
    delete objPool;
}

void TcpServer::Open()
{
    if (eventFd > 0)
    {
        return;
    }
    if ((eventFd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:socket() failed", name.c_str()));
    }
    SetNonblocking(eventFd);

    memset(&myserver, 0, sizeof(myserver));
    myserver.sin_family = AF_INET;
    myserver.sin_addr.s_addr = htonl(INADDR_ANY);
    myserver.sin_port = htons(listenPort);

    int reuse = 1;
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:setsockopt(SO_REUSEADDR) failed", name.c_str()));
    }
    if (setsockopt(eventFd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse, sizeof(reuse)) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:setsockopt(SO_REUSEPORT) failed", name.c_str()));
    }

    if (bind(eventFd, (sockaddr *)&myserver, sizeof(myserver)) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:bind() failed", name.c_str()));
    }

    if (listen(eventFd, objPool->Size()) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:listen() failed", name.c_str()));
    }
    events = EPOLLIN | EPOLLET;
    Epoll::Instance().AddEvent(this, events);
    DebugLog("Starting %s listener on 0.0.0.0:%d", TcpSvrTypeName(serverType).c_str(), listenPort);
}

void TcpServer::Close()
{
    if (eventFd > 0)
    {
        auto &busy = objPool->BusyObj();
        for (auto &s : busy)
        {
            if (s != nullptr)
            {
                s->Release(); // s is OprTcp*
            }
        }
        Epoll::Instance().DeleteEvent(this, events);
        close(eventFd);
        eventFd = -1;
    }
}

void TcpServer::EventsHandle(uint32_t events)
{
    if (events & EPOLLIN)
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
    DebugPrt("%s:Incomming...", name.c_str());
    int connfd = accept(eventFd, (sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:accept() failed", name.c_str()));
    }
    OprTcp *tcpOperator = objPool->Pop();
    if (tcpOperator == nullptr)
    {
        close(connfd);
        DebugLog("%s:Connection pool is full, reject", name.c_str());
        return;
    }
    SetNonblocking(connfd);
    char ip_port[32];
    snprintf(ip_port, 31, "%s:%d", inet_ntoa(clientaddr.sin_addr), clientaddr.sin_port);
    tcpOperator->Accept(connfd, tmrEvt, ip_port);
    DebugPrt("%s:Accept %s as %s", name.c_str(), ip_port, tcpOperator->Name().c_str());
}

void TcpServer::SetNonblocking(int sock)
{
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:SetNonblocking()-fcntl(sock, F_GETFL) failed", name.c_str()));
    }
    opts = opts | O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        throw runtime_error(StrFn::PrintfStr("%s:SetNonblocking()-fcntl(sock, F_SETFL) failed", name.c_str()));
    }
}
