#ifndef REDISPROXY_H
#define REDISPROXY_H

//Application version
#define APP_VERSION "2.0"
#define APP_NAME "OneValue"
#define APP_EXIT_KEY 10

#include "util/tcpserver.h"
#include "command.h"
#include "redisproto.h"
#include "leveldb.h"

class RedisProxy;
class ClientPacket : public Context
{
public:
    enum State {
        Unknown = 0,
        ProtoError = 1,
        ProtoNotSupport = 2,
        WrongNumberOfArguments = 3,
        RequestError = 4,
        RequestFinished = 5
    };

    ClientPacket(void) {
        commandType = -1;
        recvBufferOffset = 0;
    }

    ~ClientPacket(void) {}

    void setFinishedState(State state);
    RedisProxy* proxy(void) const
    { return (RedisProxy*)server; }

    int commandType;
    int recvBufferOffset;
    RedisProtoParseResult recvParseResult;
};



class Monitor
{
public:
    Monitor(void) {}
    virtual ~Monitor(void) {}

    virtual void proxyStarted(RedisProxy*) {}
    virtual void clientConnected(ClientPacket*) {}
    virtual void clientDisconnected(ClientPacket*) {}
    virtual void replyClientFinished(ClientPacket*) {}
};


class Sync;
class RedisProxy : public TcpServer
{
public:
    enum {
        DefaultPort = 8221
    };

    RedisProxy(void);
    ~RedisProxy(void);

public:
    void setEventLoopThreadPool(EventLoopThreadPool* pool)
    { m_eventLoopThreadPool = pool; }

    EventLoopThreadPool* eventLoopThreadPool(void)
    { return m_eventLoopThreadPool; }

    bool vipEnabled(void) const { return m_vipEnabled; }
    const char* vipName(void) const { return m_vipName; }
    const char* vipAddress(void) const { return m_vipAddress; }
    void setVipName(const char* name) { strcpy(m_vipName, name); }
    void setVipAddress(const char* address) { strcpy(m_vipAddress, address); }
    void setVipEnabled(bool b) { m_vipEnabled = b; }

    void setUnixSockFileName(const char* file) { strcpy(m_unixSocketFileName, file); }
    const char* unixSocketFileName(void) const { return m_unixSocketFileName; }

    void setMonitor(Monitor* monitor);
    Monitor* monitor(void) const { return m_monitor; }

    LeveldbCluster* leveldbCluster(void) { return m_leveldbCluster; }
    void setLeveldbCluster(LeveldbCluster* db) { m_leveldbCluster = db; }

    void setSyncThread(Sync* sync) { m_syncThread = sync; }
    Sync* syncThread(void) const { return m_syncThread; }

    bool run(const HostAddress &addr);
    void stop(void);

    virtual Context* createContextObject(void);
    virtual void destroyContextObject(Context* c);
    virtual void closeConnection(Context* c);
    virtual void clientConnected(Context* c);
    virtual ReadStatus readingRequest(Context* c);
    virtual void readRequestFinished(Context* c);
    virtual void writeReply(Context* c);
    virtual void writeReplyFinished(Context* c);

private:
    static void vipHandler(socket_t, short, void*);

private:
    Monitor* m_monitor;
    LeveldbCluster* m_leveldbCluster;
    Sync* m_syncThread;
    char m_vipName[256];
    char m_vipAddress[256];
    TcpSocket m_vipSocket;
    char m_unixSocketFileName[256];
    Event m_sockfileEvent;
    TcpSocket m_sockfile;
    Event m_vipEvent;
    bool m_vipEnabled;
    unsigned int m_threadPoolRefCount;
    EventLoopThreadPool* m_eventLoopThreadPool;
};

#endif
