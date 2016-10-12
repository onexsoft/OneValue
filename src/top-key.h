#ifndef ONE_CACHE_TOPKEY
#define ONE_CACHE_TOPKEY

#include <map>
#include <list>
#include <queue>
#include <string.h>
#include <algorithm>
#include <unordered_map>
#include "util/thread.h"
#include "util/locker.h"
using namespace std;



struct hash_func {
    unsigned int operator()(const char *str) const {
        unsigned int hash = 0;
        unsigned int x    = 0;
        while (*str) {
            hash = (hash << 4) + (*str++);
            if ((x = hash & 0xF0000000L) != 0) {
                hash ^= (x >> 24);
                hash &= ~x;
            }
        }
        return (hash & 0x7FFFFFFF);
    }
};

struct cmp_fun {
    bool operator()(const char* str1, const char* str2) const {
        return (strcmp(str1, str2) == 0);
    }
};

class KeyStrValueSize {
public:
    char* key;
    unsigned long valueSize;
};


class KeyCntValueSize {
public:
    KeyCntValueSize(){
        valueSize = 0LL;
        keyCnt = 0LL;
    }
    unsigned long valueSize;
    unsigned long keyCnt;
};

class CTopKeyRecorderThread : public Thread
{
public:
    typedef list <KeyCntValueSize> KeyCntList;
    typedef queue <KeyStrValueSize> KeyList;
    typedef unordered_map <const char*, KeyCntList::iterator, hash_func, cmp_fun> KeyCntMap;
    CTopKeyRecorderThread(int cnt = 5000000) {
        m_processTopKey = false;
        m_maxKeyCnt     = cnt;
    }
    ~CTopKeyRecorderThread(){
    }

    void keyRecorder(KeyStrValueSize&);
    void keyToList(KeyStrValueSize&);

protected:
    virtual void run(void);
    virtual void onExit(void);

public:
    void delListMap();
    KeyCntMap::iterator minCntMap(KeyCntList::iterator);
    bool         m_processTopKey;
    KeyList      m_allKeys;
    unsigned int m_maxKeyCnt;
    KeyCntList   m_keyCntList;
    KeyCntMap    m_keyCntMap;
    Mutex       m_lock;
    friend class CTopKeySorter;
};


class CTopKeySorter {
public:
    CTopKeySorter();
    ~CTopKeySorter();
    void sortTopKeyByCnt(CTopKeyRecorderThread&);
    void sortTopKeyByValueSize(CTopKeyRecorderThread&);

    vector<pair <const char*, CTopKeyRecorderThread::KeyCntList::iterator> >* m_sortVectorKeyCnt;
};

#endif
