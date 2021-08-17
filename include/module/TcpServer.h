#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include <unistd.h>
#include <string>
#include <vector>
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
    TcpServer(int listenPort, ObjectPool<OprTcp> & oPool, TimerEvent * tmr);
    ~TcpServer();

    /// \brief  Incoming... Accept
    void Accept();

    /// \brief  Event triggered
    void EventsHandle(uint32_t events);

    void Release(OprTcp * tcpOperator);

private:
    int listenPort;
    std::string name;
    ObjectPool<OprTcp> & oPool;
    TimerEvent * tmrEvt;
    sockaddr_in myserver;
    void SetNonblocking(int sock);
};

#endif
