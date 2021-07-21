#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include <unistd.h>
#include <string>
#include <vector>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include "IGcEvent.h"
#include "ILowerLayer.h"
#include "TcpOperator.h"

#define MAX_CLIENT 10


class TcpServer : IGcEvent
{
public:
    TcpServer(std::string name, int listenPort, int clientMax, ILowerLayer::LowerLayerType llType);
    ~TcpServer();

    /// \brief  Incoming... Accept
    void Accept();

    /// \brief  Event triggered
    void EventsHandle(uint32_t events);

private:
    std::string name;
    int listenPort;
    ILowerLayer::LowerLayerType llType;
    int clientMax;
    int clientActive;
    sockaddr_in myserver;

    void SetNonblocking(int sock);
    std::vector<TcpOperator *> clients;
};

#endif
