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

#define MAX_CLIENT 10

class Client : IGcEvent
{
public:
    Client(std::string name, int fd, ILowerLayer::LowerLayerType llType);
    ~Client();
    void InEvent();
    void OutEvent();
    void Error(uint32_t events);
    int GetFd();
private:
    std::string name;
    ILowerLayer::LowerLayerType llType;
    ILowerLayer *lowerLayer;
    int clientFd;
    uint32_t events;
    void Release();
};

class TcpServer : IGcEvent
{
public:
    TcpServer(int listenPort, int clientMax, ILowerLayer::LowerLayerType llType);
    ~TcpServer();

    void InEvent();

    void OutEvent();

    void Error(uint32_t events);
    
    int GetFd();

private:
    int listenPort;
    ILowerLayer::LowerLayerType llType;
    int serverFd;
    int clientMax;
    int clientActive;
    sockaddr_in myserver;

    void SetNonblocking(int sock);
    std::vector<Client *> clients;
};

#endif
