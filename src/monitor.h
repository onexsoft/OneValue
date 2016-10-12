#ifndef ONE_PROXY_CACHE_MONITOR
#define ONE_PROXY_CACHE_MONITOR

#include <map>
#include <string>
#include <time.h>

//#include "redisservant.h"
#include "redisproxy.h"
#include "top-key.h"
#include "onevaluecfg.h"

// 记录所有命令使用的次数
class CCommandRecorder {
public:
    CCommandRecorder();
    ~CCommandRecorder();
    inline void addCnt(int commandType);
    inline unsigned long long commandCount(int cmdType) const
    { return m_cmdCnt[cmdType]; }

    void reset(){  }
private:
     unsigned long long m_cmdCnt[RedisCommand::CMD_COUNT];
};


//字节统计类
class CByteCounter
{
public:
    enum {
        KBSIZE = 1024,
        MBSIZE = 1048576,
        GBSIZE = 1073741824
    };
    CByteCounter();
    ~CByteCounter();

    void addBytes(int bytes);
    int queryKBRange(int left, int right);
    int queryMBRange(int left, int right);
    int queryGBRange(int left, int right);
private:
    int gb_array[1024];
    int mb_array[1024];
    int kb_array[1024];
};


class CTimming {
public:
    void timmingBegin();
    void timmingEnd();
    static char* toString(time_t t);
    static char* secondToString(int sec);
    time_t beginTime();
    time_t endTime();
private:
    time_t begin_time;
    time_t end_time;
};


class CIpUtil {
public:
    // int类型ip转换成字符串格式ip
    static char* int2ipstr (int ip);
};

// 记录每个客户端连接信息 包括:
// 客户端ip 同一个ip的连接次数  请求次数
// 记录所有命令 记录value的超过1k等大小的次数
class SClientRecorder
{
public:
    SClientRecorder();
    ~SClientRecorder();
    void reset(){}

    inline int clientIp() {return m_clientIp;}
    inline long requestNum() {return request_num;}
    inline long connectNum() {return connect_num;}
public:
    int m_clientIp;
    long request_num; // 同一个客户端请求次数
    long connect_num; // 同一个客户端连接次数
    CCommandRecorder commandRecorder;
    CByteCounter valueInfo;
    CTimming connectInfo;
    friend class CProxyMonitor;
};



class CProxyMonitor : public Monitor
{
public:
    CProxyMonitor();
    ~CProxyMonitor();
    typedef std::map<int, SClientRecorder> ClientRecorderMap; // key: client's ip

    virtual void proxyStarted(RedisProxy*);
    virtual void clientConnected(ClientPacket*);
    virtual void clientDisconnected(ClientPacket*);
    virtual void replyClientFinished(ClientPacket*);
public:
    time_t proxyBeginTime()              { return m_proxyBeginTime.beginTime(); }
    ClientRecorderMap* clientRecordMap() { return &m_clientRecMap; }
    RedisProxy* redisProxy()             { return m_redisProxy; }
    CTopKeyRecorderThread* topKeyRecorder()    { return &m_topKeyRecorderThread; }
public:
    bool m_topKeyEnable;
private:
    // for client
    ClientRecorderMap      m_clientRecMap;

    // for proxy self
    CTimming               m_proxyBeginTime;
    RedisProxy*            m_redisProxy;
    SpinLocker             m_clientLock;
    SpinLocker             m_backendLock;
    SpinLocker             m_topKeyLock;

    CTopKeyRecorderThread  m_topKeyRecorderThread;
};


class CFileOperate {
public:
    static FILE* openFile(const char* path, const char* mode) {
        return fopen(path, mode);
    }
    static void formatString2File(const char* string, FILE* f){ fprintf(f,"%s",string); }
};

class CFormatMonitorToIoBuf {
public:
    CFormatMonitorToIoBuf(IOBuffer* pIobuf, int topKeyCnt = 20);
    ~CFormatMonitorToIoBuf(){}
    void formatProxyToIoBuf(CProxyMonitor& proxyMonirot); // 将proxy信息格式化到m_iobuf
    void formatClientsToIoBuf(CProxyMonitor& proxyMonirot); // 将client信息格式化到m_iobuf

    void formatTopKeyToIoBuf(CProxyMonitor& proxyMonirot);
    void formatTopValueToIoBuf(CProxyMonitor& proxyMonirot);

    IOBuffer*      m_iobuf;
    FILE*          m_pfile;
    CTopKeySorter  m_topKeySorter;
    int            m_topKeyCnt;
};

class CShowMonitor {
public:
    static void showMonitorToIobuf(CFormatMonitorToIoBuf& formatMonitor, CProxyMonitor& monitor);
    static bool showMonitorToFile(const char* p, const char* mode, CFormatMonitorToIoBuf& f, CProxyMonitor& m);
};


#endif
