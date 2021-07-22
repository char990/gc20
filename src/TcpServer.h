#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include <unistd.h>
#include <string>
#include <vector>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "IGcEvent.h"
#include "IAdaptLayer.h"
#include "TcpOperator.h"
#include "ObjectPool.h"

#define MAX_CLIENT 10

class TcpServer : IGcEvent
{
public:
    TcpServer(int listenPort, ObjectPool<TcpOperator> & oPool);
    ~TcpServer();

    /// \brief  Incoming... Accept
    void Accept();

    /// \brief  Event triggered
    void EventsHandle(uint32_t events);

private:
    int listenPort;
    std::string name;
    ObjectPool<TcpOperator> & oPool;
    sockaddr_in myserver;

    void SetNonblocking(int sock);
};

#endif
