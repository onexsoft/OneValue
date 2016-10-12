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


#ifndef BINLOG_H
#define BINLOG_H

#include <string>
#include <vector>

#include "util/locker.h"

class TextConfigFile
{
public:
    static bool read(const std::string& fname, std::vector<std::string>& values);
    static bool write(const std::string& fname, const std::vector<std::string>& values);
};


class BinlogFileList
{
public:
    BinlogFileList(void);
    ~BinlogFileList(void);

    void appendFileName(const std::string& fileName)
    { m_binlogFiles.push_back(fileName); }
    bool isEmpty(void) const { return m_binlogFiles.empty(); }
    int fileCount(void) const { return m_binlogFiles.size(); }
    std::string fileName(int index) const { return m_binlogFiles[index]; }
    int indexOfFileName(const std::string& fileName) const;

    static void loadFromFile(BinlogFileList* index, const std::string& fname);
    static void saveToFile(BinlogFileList* index, const std::string& fname);

private:
    std::vector<std::string> m_binlogFiles;
    BinlogFileList(const BinlogFileList&);
    BinlogFileList& operator=(const BinlogFileList&);
};


class Binlog
{
public:
    struct Header {
        enum { _handshake = 0x1ecdf00 };
        int handshake;

        bool isValid(void) const
        { return handshake == _handshake; }
    };

    struct LogItem {
        enum {
            SET,
            DEL
        };
        int item_size;
        int type;
        int key_size;
        int value_size;

        char* keyBuffer(void) { return (char*)(this+1); }
        char* valueBuffer(void) { return keyBuffer()+key_size; }
    };

    Binlog(void);
    ~Binlog(void);

    bool open(const std::string& fname);

    std::string fileName(void) const { return m_fileName; }
    size_t writtenSize(void) const { return m_writtenSize; }

    void sync(void);
    void close(void);

    bool appendSetRecord(const char* key, int klen, const char* value, int vlen);
    bool appendDelRecord(const char* key, int klen);

private:
    std::string m_fileName;
    FILE* m_fp;
    size_t m_writtenSize;
    Binlog(const Binlog&);
    Binlog& operator=(const Binlog&);
};


struct BinlogSyncStream {
    enum Error {
        NoError = 0,
        InvalidFileName
    };

    enum {
        MaxStreamSize = 32*1024*1024  //max stream size. 32M
    };

    char ch;                //+
    int streamSize;         //stream size
    int error;              //error
    char errorMsg[128];     //error message
    char srcFileName[128];  //last binlog file name
    int lastUpdatePos;      //last update in the binlog
    int logItemCount;       //item count

    Binlog::LogItem* firstLogItem(void) {
        return (Binlog::LogItem*)(this+1);
    }

    Binlog::LogItem* nextLogItem(Binlog::LogItem* cur) {
        return (Binlog::LogItem*)(((char*)cur)+cur->item_size);
    } 
};

class BinlogBufferReader
{
public:
    BinlogBufferReader(void) :
        m_buff(NULL),
        m_len(0)
    {}
    BinlogBufferReader(char* buff, int len) :
        m_buff(buff),
        m_len(len)
    {}
    ~BinlogBufferReader(void)
    {}

    Binlog::LogItem* firstItem(void) const {
        if (m_len == 0) {
            return NULL;
        }
        return (Binlog::LogItem*)(m_buff);
    }
    Binlog::LogItem* nextItem(Binlog::LogItem* item) const {
        if ((char*)item + item->item_size - m_buff >= m_len) {
            return NULL;
        }
        return (Binlog::LogItem*)(((char*)item)+item->item_size);
    }
private:
    char* m_buff;
    int m_len;
};


class BinlogParser
{
public:
    BinlogParser(void);
    ~BinlogParser(void);

    bool open(const std::string& fname);
    BinlogBufferReader reader(void) const;
    void close(void);

private:
    char* m_base;
    int m_size;
    int m_fd;

    BinlogParser(const BinlogParser&);
    BinlogParser& operator=(const BinlogParser&);
};

#endif
