/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

#include <time.h>

#include "leveldb.h"
#include "util/logger.h"
#include "ttlmanager.h"

struct KeyHeader {
    int timestamp;
};

#ifndef WIN32
class OneValueComparator : public leveldb::Comparator
{
public:
    OneValueComparator(void) {}
    ~OneValueComparator(void) {}

    virtual int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
        leveldb::Slice _a(a.data() + sizeof(KeyHeader), a.size() - sizeof(KeyHeader));
        leveldb::Slice _b(b.data() + sizeof(KeyHeader), b.size() - sizeof(KeyHeader));
        return _a.compare(_b);
    }

    virtual const char* Name(void) const {
        return "OneValue.Comparator";
    }

    virtual void FindShortestSeparator(std::string* start, const leveldb::Slice& limit) const
    {
        (void)start;
        (void)limit;
    }

    virtual void FindShortSuccessor(std::string* key) const {
        (void)key;
    }

    static leveldb::Comparator* defaultComparator(void) {
        static OneValueComparator* c = NULL;
        if (!c) {
            c = new OneValueComparator;
        }
        return c;
    }
};
#endif



Leveldb::Leveldb(void)
{
#ifndef WIN32
    m_db = NULL;
#endif
}

Leveldb::~Leveldb(void)
{
#ifndef WIN32
    if (m_db) {
        delete m_db;
    }

    if (m_options.block_cache) {
        delete m_options.block_cache;
    }
#endif
}

bool Leveldb::open(const std::string &name, const Option& opt)
{
#ifndef WIN32
    m_options.create_if_missing = true;
    if (opt.compress) {
        m_options.compression = leveldb::kSnappyCompression;
    } else {
        m_options.compression = leveldb::kNoCompression;
    }

    if (opt.cacheSize > 0) {
        m_options.block_cache = leveldb::NewLRUCache(opt.cacheSize);
    }

    if (opt.writeBufferSize > 0) {
        m_options.write_buffer_size = opt.writeBufferSize;
    }

    //m_options.comparator = OneValueComparator::defaultComparator();

    leveldb::Status status = leveldb::DB::Open(m_options, name, &m_db);
    if (status.ok()) {
        m_dbName = name;
        m_opt = opt;
        Logger::log(Logger::Message, "leveldb '%s' opened. compress=%s cache_size=%uMB write_buffer_size=%uMB",
                        m_dbName.c_str(),
                        m_opt.compress ? "true" : "false",
                        m_opt.cacheSize / 1024 / 1024,
                        m_opt.writeBufferSize / 1024 / 1024);
        return true;
    }
    return false;
#else
    (void)name;
    (void)opt;
    return false;
#endif
}

bool Leveldb::isOpened(void) const
{
#ifndef WIN32
    return (m_db == NULL);
#else
    return false;
#endif
}

bool Leveldb::initIterator(LeveldbIterator &iter)
{
#ifdef WIN32
    (void)iter;
    return false;
#else
    leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());
    if (!it) {
        return false;
    }
    if (iter.m_iter) {
        delete iter.m_iter;
    }
    iter.m_iter = it;
    iter.m_db = this;
    return true;
#endif
}

bool Leveldb::setValue(const XObject& key, const XObject& val, bool sync)
{
#ifndef WIN32
    leveldb::Slice _key(key.data, key.len);
    leveldb::Slice _value(val.data, val.len);
    leveldb::WriteOptions options;
    options.sync = sync;
    leveldb::Status status = m_db->Put(options, _key, _value);
    return status.ok();
#else
    (void)key;
    (void)val;
    (void)sync;
    return false;
#endif
}

bool Leveldb::value(const XObject& key, std::string &val)
{
#ifndef WIN32
    leveldb::Slice _key(key.data, key.len);
    leveldb::ReadOptions options;
    leveldb::Status status = m_db->Get(options, _key, &val);
    return status.ok();
#else
    (void)key;
    (void)val;
    return false;
#endif
}

bool Leveldb::remove(const XObject& key, bool sync)
{
#ifndef WIN32
    leveldb::Slice _key(key.data, key.len);
    leveldb::WriteOptions options;
    options.sync = sync;
    leveldb::Status status = m_db->Delete(options, _key);
    return status.ok();
#else
    (void)key;
    (void)sync;
    return false;
#endif
}

void Leveldb::clear(void)
{
    LeveldbIterator it;
    if (!initIterator(it)) {
        return;
    }

    it.seekToFirst();
    while (it.isValid()) {
        remove(it.key());
        it.next();
    }

    remove(XObject("TEST", 4), true);
}



static bool fileExists(const std::string& file)
{
#ifndef WIN32
    return leveldb::Env::Default()->FileExists(file);
#else
    (void)file;
    return false;
#endif
}

static bool createDirectory(const std::string& dir)
{
#ifndef WIN32
    return leveldb::Env::Default()->CreateDir(dir).ok();
#else
    (void)dir;
    return false;
#endif
}


LeveldbCluster::LeveldbCluster(void)
{
    for (int i = 0; i < MaxHashValue; ++i) {
        m_hashMapping[i] = NULL;
    }
    m_started = false;
    m_ttlManager = new TTLManager(this);
}

LeveldbCluster::~LeveldbCluster(void)
{
    delete m_ttlManager;
    stop();
}

Leveldb *LeveldbCluster::database(const std::string &name) const
{
    std::string fullPath = subFileName(name);
    for (unsigned int i = 0; i < m_dbs.size(); ++i) {
        Leveldb* db = m_dbs[i];
        if (db->databaseName() == fullPath) {
            return db;
        }
    }
    return NULL;
}

std::string LeveldbCluster::subFileName(const std::string &fileName) const
{
    return m_option.workdir + "/" + fileName;
}

std::string LeveldbCluster::binlogListFileName(void) const
{
    return subFileName("binlog/BINLOG_INDEX");
}

std::string LeveldbCluster::binlogFileName(const std::string &baseName) const
{
    return subFileName("binlog/" + baseName);
}

bool LeveldbCluster::setMapping(int hashValue, const std::string &dbName)
{
    Leveldb* db = database(dbName);
    if (hashValue >= 0 && hashValue < MaxHashValue && db) {
        m_hashMapping[hashValue] = db;
        return true;
    }
    return false;
}

bool LeveldbCluster::start(const LeveldbCluster::Option& opt)
{
    if (m_started) {
        return true;
    }

    Logger::log(Logger::Message, "Start database...");

    //Create work directory
    std::string workdir = opt.workdir;
    if (workdir.empty()) {
        Logger::log(Logger::Warning, "Working directory is empty, default use the current directory");
        workdir = ".";
    } else {
        if (!fileExists(workdir)) {
            if (!createDirectory(workdir)) {
                Logger::log(Logger::Warning, "Unable to create a working directory '%s', "
                                "default use the current directory", workdir.c_str());
                workdir = ".";
            }
        }
    }

    //Create database
    for (unsigned int i = 0; i < opt.dbnames.size(); ++i) {
        std::string fullPath = workdir + "/" + opt.dbnames[i];
        Leveldb* db = new Leveldb;
        if (!db->open(fullPath, opt.leveldbopt)) {
            Logger::log(Logger::Error, "Open database '%s' failed", fullPath.c_str());
            stop();
            return false;
        }
        m_dbs.push_back(db);
    }

    m_option = opt;
    m_option.workdir = workdir;
    m_started = true;

    if (m_option.binlogEnabled) {
        initBinlog();
    }

    m_ttlManager->start();

    Logger::log(Logger::Message, "Database started. workdir=%s maxhash=%d sync=%s "
                    "binlog_enabled=%s max_binlog_size=%dMB",
                    m_option.workdir.c_str(),
                    m_option.maxhash,
                    m_option.sync ? "true" : "false",
                    m_option.binlogEnabled ? "true" : "false",
                    m_option.maxBinlogSize / 1024 / 1024);
    return true;
}

void LeveldbCluster::stop(void)
{
    if (m_started) {
        Logger::log(Logger::Message, "Stop database...");
        for (int i = 0; i < MaxHashValue; ++i) {
            m_hashMapping[i] = NULL;
        }

        /* stop the ttl manager before the database shutdown, else it may coredumped */
        m_ttlManager->stop();
        
        for (unsigned int i = 0; i < m_dbs.size(); ++i) {
            Leveldb* db = m_dbs[i];
            delete db;
        }

        m_curBinlog.close();
        m_started = false;
        Logger::log(Logger::Message, "Database stopped");
    }
}

Leveldb* LeveldbCluster::mapToDatabase(const char* key, int len) const
{
    unsigned int hash_val = m_option.hashfunc(key, len);
    unsigned int idx = hash_val % m_option.maxhash;
    return m_hashMapping[idx];
}

std::string LeveldbCluster::buildRandomBinlogFileBaseName(void) const
{
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    char buff[1024];
    sprintf(buff, "%d%02d%02d_%02d%02d%02d-%d-bin",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec, rand() % 100000);
    return buff;
}

bool LeveldbCluster::setValue(const XObject& key, const XObject& val, const WriteOption& opt)
{
    XObject _mapping = opt.mapping_key.isNull() ? key : opt.mapping_key;
    Leveldb* db = mapToDatabase(_mapping.data, _mapping.len);
    if (!db) {
        return false;
    }
    bool ok = db->setValue(key, val, m_option.sync);
    if (ok && m_option.binlogEnabled) {
        lockCurrentBinlogFile();
        m_curBinlog.appendSetRecord(key.data, key.len, val.data, val.len);
        ajustCurrentBinlogFile();
        unlockCurrentBinlogFile();
    }
    return ok;
}

bool LeveldbCluster::value(const XObject& key, std::string &val, const ReadOption& opt)
{
    XObject _mapping = opt.mapping_key.isNull() ? key : opt.mapping_key;
    Leveldb* db = mapToDatabase(_mapping.data, _mapping.len);
    if (!db) {
        return false;
    }
    return db->value(key, val);
}

bool LeveldbCluster::remove(const XObject& key, const WriteOption& opt)
{
    XObject _mapping = opt.mapping_key.isNull() ? key : opt.mapping_key;
    Leveldb* db = mapToDatabase(_mapping.data, _mapping.len);
    if (!db) {
        return false;
    }
    bool ok = db->remove(key, m_option.sync);
    if (ok && m_option.binlogEnabled) {
        lockCurrentBinlogFile();
        m_curBinlog.appendDelRecord(key.data, key.len);
        ajustCurrentBinlogFile();
        unlockCurrentBinlogFile();
    }
    return ok;
}

void LeveldbCluster::clear(void)
{
    for (int i = 0; i < databaseCount(); ++i) {
        database(i)->clear();
    }
}

bool LeveldbCluster::initBinlog(void)
{
    const std::string binlogDir = subFileName("binlog");
    if (!fileExists(binlogDir)) {
        if (!createDirectory(binlogDir)) {
            Logger::log(Logger::Error, "initBinlog: Can't create binlog directory '%s'", binlogDir.c_str());
            return false;
        }
    }

    const std::string indexFile = binlogListFileName();
    BinlogFileList::loadFromFile(&m_binlogFileList, indexFile);

    std::string randomFileName;
    std::string lastBinlogFile;
    int fileCount = m_binlogFileList.fileCount();
    if (fileCount == 0) {
        randomFileName = buildRandomBinlogFileBaseName();
        lastBinlogFile = binlogFileName(randomFileName);
    } else {
        lastBinlogFile = binlogFileName(m_binlogFileList.fileName(fileCount-1));
    }

    Logger::log(Logger::Message, "Open binlog file '%s'...", lastBinlogFile.c_str());
    if (!m_curBinlog.open(lastBinlogFile)) {
        Logger::log(Logger::Error, "Can't open binlog file '%s'", lastBinlogFile.c_str());
        return false;
    }

    if (!randomFileName.empty()) {
        m_binlogFileList.appendFileName(randomFileName);
        BinlogFileList::saveToFile(&m_binlogFileList, indexFile);
    }

    return true;
}

void LeveldbCluster::ajustCurrentBinlogFile(void)
{
    if (m_curBinlog.writtenSize() >= m_option.maxBinlogSize) {
        m_curBinlog.close();
        std::string newFileBaseName = buildRandomBinlogFileBaseName();
        std::string fullPath = binlogFileName(newFileBaseName);
        if (m_curBinlog.open(fullPath)) {
            m_binlogFileList.appendFileName(newFileBaseName);
            BinlogFileList::saveToFile(&m_binlogFileList, binlogListFileName());
        }
    }
}
