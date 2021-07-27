#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include <unistd.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "IGcEvent.h"
#include "OprTcp.h"
#include "ObjectPool.h"

class TcpServer : IGcEvent
{
public:
    TcpServer(int listenPort, ObjectPool<OprTcp> & oPool);
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
    sockaddr_in myserver;
    void SetNonblocking(int sock);
};

#endif
