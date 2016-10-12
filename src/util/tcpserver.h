#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "iobuffer.h"
#include "eventloop.h"
#include "tcpsocket.h"

class TcpServer;
class Context
{
public:
    Context(void) {
        server = NULL;
        sendBytes = 0;
        recvBytes = 0;
        eventLoop = NULL;
    }

    virtual ~Context(void) {}

    TcpSocket clientSocket;     //Client socket
    HostAddress clientAddress;  //Client address
    TcpServer* server;          //The Connected server
    IOBuffer sendBuff;          //Send buffer
    IOBuffer recvBuff;          //Recv buffer
    int sendBytes;              //Current send bytes
    int recvBytes;              //Current recv bytes
    EventLoop* eventLoop;       //Use the event loop
    Event _event;               //Read/Write event
};


class TcpServer
{
public:
    enum ReadStatus {
        ReadFinished = 0,
        ReadIncomplete = 1,
        ReadError = 2
    };

    TcpServer(void);
    virtual ~TcpServer(void);

    EventLoop* eventLoop(void) { return &m_loop; }

    const HostAddress& address(void) const { return m_addr; }
    bool run(const HostAddress& addr);
    bool isRunning(void) const;
    void stop(void);

    virtual Context* createContextObject(void);
    virtual void destroyContextObject(Context* c);
    virtual void closeConnection(Context* c);
    virtual void clientConnected(Context* c);
    virtual void waitRequest(Context* c);
    virtual ReadStatus readingRequest(Context* c);
    virtual void readRequestFinished(Context* c);
    virtual void writeReply(Context* c);
    virtual void writeReplyFinished(Context* c);

protected:
    static void onAcceptHandler(evutil_socket_t sock, short, void* arg);

private:
    HostAddress m_addr;
    Event m_listener;
    EventLoop m_loop;
    TcpSocket m_socket;

private:
    TcpServer(const TcpServer&);
    TcpServer& operator=(const TcpServer&);
};

#endif
