#ifndef TTLMANAGER_H
#define TTLMANAGER_H

#include "util/thread.h"
#include "util/iobuffer.h"
#include "leveldb.h"


struct ExpireKey {
    ExpireKey(){ type = -1; keyLen = 0;}
    short type;
    unsigned int keyLen;
    char* keyBuf() const {
        return (char*)(this + 1);
    }
    std::string key()const {
        return std::string((char*)(this + 1), keyLen);
    }
    static void makeExpireKey(IOBuffer& buf, const XObject& key);
};

class TTLThread;
class TTLManager
{
public:
    TTLManager(LeveldbCluster* dbCluster);
    ~TTLManager();

    LeveldbCluster* leveldbCluster(void) { return m_dbCluster; }
    bool setExpire(const XObject& key, unsigned int seconds);
    void start();
    void stop();

private:
    LeveldbCluster*  m_dbCluster;
    TTLThread*       m_ttlThread;
    friend class     TTLThread;
    TTLManager(const TTLManager& rhs);
    TTLManager& operator=(const TTLManager& rhs);
};


#endif // TTLMANAGER_H



