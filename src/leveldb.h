#ifndef LEVELDB_H
#define LEVELDB_H

#include <string>
#include <vector>

#include "util/hash.h"
#include "binlog.h"

#ifndef WIN32
#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/cache.h>
#include <leveldb/comparator.h>
#endif

class XObject;
class Leveldb;
class LeveldbIterator;
class LeveldbCluster;
class TTLManager;

class XObject
{
public:
    XObject(void) {
        data = NULL;
        len = 0;
    }
    XObject(char* p, int len) :
        data(p),
        len(len)
    {}
    XObject(const char* p, int len) :
        data((char*)p),
        len(len)
    {}
    ~XObject(void)
    {}

    bool isNull(void) const
    { return data == NULL; }

    void dump(void) {
        for (int i = 0; i < len; ++i)
            printf("%d ", data[i]);
        printf("\n");
    }

    char* data;
    int len;
};


class LeveldbIterator
{
public:
    LeveldbIterator(void) {
#ifndef WIN32
        m_iter = NULL;
#endif
        m_db = NULL;
    }
    ~LeveldbIterator(void) {
#ifndef WIN32
        if (m_iter) {
            delete m_iter;
        }
#endif
    }
    Leveldb* database(void) const { return m_db; }
    bool isValid(void) const {
#ifndef WIN32
        return m_iter->Valid();
#else
        return false;
#endif
    }
    void seekToFirst(void) {
#ifndef WIN32
        return m_iter->SeekToFirst();
#endif
    }
    void seekToLast(void) {
#ifndef WIN32
        return m_iter->SeekToLast();
#endif
    }
    void seek(const XObject& target) {
#ifndef WIN32
        return m_iter->Seek(leveldb::Slice(target.data, target.len));
#else
        (void)target;
#endif
    }
    void next(void) {
#ifndef WIN32
        return m_iter->Next();
#endif
    }
    void prev(void) {
#ifndef WIN32
        return m_iter->Prev();
#endif
    }
    XObject key(void) const {
#ifndef WIN32
        leveldb::Slice _key = m_iter->key();
        return XObject((char*)_key.data(), _key.size());
#else
        return XObject();
#endif
    }
    XObject value(void) const {
#ifndef WIN32
        leveldb::Slice _val = m_iter->value();
        return XObject((char*)_val.data(), _val.size());
#else
        return XObject();
#endif
    }

private:
#ifndef WIN32
    leveldb::Iterator* m_iter;
#endif
    Leveldb* m_db;
    LeveldbIterator(const LeveldbIterator&);
    LeveldbIterator& operator=(const LeveldbIterator &);
    friend class Leveldb;
};

class Leveldb
{
public:
    struct Option {
        bool compress;
        size_t cacheSize;
        size_t writeBufferSize;

        Option(void) {
            compress = false;
            cacheSize = 0;
            writeBufferSize = 4*1024*1024;
        }
        ~Option(void) {}
    };

    Leveldb(void);
    ~Leveldb(void);

    Option option(void) const { return m_opt; }
    bool open(const std::string& name, const Option& opt);

    bool isOpened(void) const;
    std::string databaseName(void) const { return m_dbName; }

    bool initIterator(LeveldbIterator& iter);

    bool setValue(const XObject& key, const XObject& val, bool sync = false);
    bool value(const XObject& key, std::string& val);
    bool remove(const XObject& key, bool sync = false);
    void clear(void);

private:
#ifndef WIN32
    leveldb::DB* m_db;
    leveldb::Options m_options;
#endif
    Option m_opt;
    std::string m_dbName;

private:
    Leveldb(const Leveldb&);
    Leveldb& operator =(const Leveldb& rhs);
};

class LeveldbCluster
{
public:
    enum { MaxHashValue = 1024 };
    struct Option {
        std::string workdir;
        std::vector<std::string> dbnames;
        int maxhash;
        HashFunc hashfunc;
        Leveldb::Option leveldbopt;
        bool sync;
        bool binlogEnabled;
        size_t maxBinlogSize;

        Option(void) {
            workdir = ".";
            maxhash = 128;
            hashfunc = hashForBytes;
            sync = false;
            binlogEnabled = false;
            maxBinlogSize = 64*1024*1024;
        }
        Option(const Option& opt) { *this = opt; }
        Option& operator =(const Option& opt) {
            if (this != &opt) {
                workdir = opt.workdir;
                dbnames = opt.dbnames;
                maxhash = opt.maxhash;
                hashfunc = opt.hashfunc;
                leveldbopt = opt.leveldbopt;
                sync = opt.sync;
                binlogEnabled = opt.binlogEnabled;
                maxBinlogSize = opt.maxBinlogSize;
            }
            return *this;
        }

        ~Option(void) {}
    };

    struct ReadOption {
        ReadOption(void) {}
        ~ReadOption(void) {}
        XObject mapping_key;
    };

    struct WriteOption {
        WriteOption(void) {}
        ~WriteOption(void) {}
        XObject mapping_key;
    };

    LeveldbCluster(void);
    ~LeveldbCluster(void);

    bool start(const Option& opt);
    bool isStarted(void) const { return m_started; }
    bool setMapping(int hashValue, const std::string& dbName);
    void stop(void);

    int databaseCount(void) const { return m_dbs.size(); }
    Leveldb* database(int index) const { return m_dbs[index]; }
    Leveldb* database(const std::string& name) const;

    BinlogFileList* binlogFileList(void) { return &m_binlogFileList; }
    Binlog* currentBinlog(void) { return &m_curBinlog; }
    TTLManager* ttlManager(void) { return m_ttlManager; }

    std::string subFileName(const std::string& fileName) const;
    std::string binlogListFileName(void) const;
    std::string binlogFileName(const std::string& baseName) const;

    Leveldb* mapToDatabase(const char* key, int len) const;

    bool setValue(const XObject& key, const XObject& val, const WriteOption& opt = WriteOption());
    bool value(const XObject& key, std::string& val, const ReadOption& opt = ReadOption());
    bool remove(const XObject& key, const WriteOption& opt = WriteOption());
    void clear(void);

    void lockCurrentBinlogFile(void) { m_binlogMutex.lock(); }
    void unlockCurrentBinlogFile(void) { m_binlogMutex.unlock(); }

private:
    bool initBinlog(void);
    void ajustCurrentBinlogFile(void);
    std::string buildRandomBinlogFileBaseName(void) const;

private:
    Option m_option;
    Leveldb* m_hashMapping[MaxHashValue];
    std::vector<Leveldb*> m_dbs;
    bool m_started;
    BinlogFileList m_binlogFileList;
    Mutex m_binlogMutex;
    Binlog m_curBinlog;
    TTLManager* m_ttlManager;

private:
    LeveldbCluster(const LeveldbCluster&);
    LeveldbCluster& operator =(const LeveldbCluster& rhs);
};


#endif
