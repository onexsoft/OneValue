#ifndef T_HASH_H
#define T_HASH_H

#include "leveldb.h"
#include "t_redis.h"
#include "util/iobuffer.h"

typedef std::pair<std::string, std::string> KeyValue;
typedef std::list<KeyValue> KeyValues;

struct HashKeyInfo
{
    short type;
    XObject name;
    XObject key;
};

class THash
{
public:
    THash(LeveldbCluster* db, const std::string& name);
    ~THash(void);

    bool hset(const std::string& field, const std::string& value);
    bool hget(const std::string& field, std::string* value);
    bool hdel(const std::string& field);
    bool hexists(const std::string& field);
    int hlen(void);
    void hgetall(KeyValues *result);
    void hgetall(stringlist* keys, stringlist* vals);
    void hclear(void);

    static void makeHashKey(IOBuffer& buf, HashKeyInfo* info);
    static void unmakeHashKey(const char* buf, int size, HashKeyInfo* info);

protected:
    Leveldb* m_db;
    short m_internalType;
    LeveldbCluster* m_dbCluster;
    std::string m_hashName;
};

class TSet : public THash
{
public:
    TSet(LeveldbCluster* db, const std::string& name) :
        THash(db, name) {
        m_internalType = T_Set;
    }
    ~TSet(void) {}
};

#endif

