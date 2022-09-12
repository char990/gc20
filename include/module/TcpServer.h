#pragma once


#include <unistd.h>
#include <string>
#include <vector>
#include <stack>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <module/IGcEvent.h>
#include <module/TimerEvent.h>
#include <module/OprTcp.h>
#include <module/ObjectPool.h>

class TcpServer : IGcEvent
{
public:
    TcpServer(int listenPort, TcpSvrType serverType, int poolsize, TimerEvent * tmr);
    ~TcpServer();

    /// \brief  Incoming... Accept
    void Accept();

    /// \brief  Event triggered
    void EventsHandle(uint32_t events);

    /// for Re-establishing : Close() - > Open()
    void Open();
    void Close();

private:
    int listenPort;
    TcpSvrType serverType;
    int poolsize;
    TimerEvent * tmrEvt;

    std::string name;

    ObjectPool<OprTcp> * objPool;

    sockaddr_in myserver;
    void SetNonblocking(int sock);
};
