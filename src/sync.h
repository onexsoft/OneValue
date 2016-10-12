#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <string>
#include "redisproxy.h"
#include "binlog.h"
#include "util/thread.h"
using namespace std;


class Sync : public Thread
{
public:
    #define RECVTIMEOUT 5000
    Sync(RedisProxy* proxy, const char* master, int port);
    ~Sync();
    void setSyncInterval(int t);
    void setReconnectInterval(int t);
protected:
    virtual void run(void);
private:
    bool _onSetCommand(Binlog::LogItem* item, LeveldbCluster* db);
    bool _onDelCommand(Binlog::LogItem* item, LeveldbCluster* db);
    void repairConnect(void);
private:
    int                      m_syncInterval;
    int                      m_reconnectInterval;
    HostAddress              m_masterAddr;
    TcpSocket                m_socket;

    RedisProxy*              m_proxy;

    string                   m_slaveIndexFileName;
    std::vector<std::string> m_masterSyncInfo;
private:
    Sync(const Sync&);
    Sync& operator =(const Sync& rhs);
};


#endif


