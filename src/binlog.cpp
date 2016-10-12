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

#ifndef WIN32
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#include "util/logger.h"
#include "util/string.h"
#include "binlog.h"

bool TextConfigFile::read(const std::string &fname, std::vector<std::string> &values)
{
    FILE* fp = fopen(fname.c_str(), "r");
    if (!fp) {
        return false;
    }

    while (!feof(fp)) {
        char lineBuff[1024] = {0};
        int len = fscanf(fp, "%[^\n]\n", lineBuff);
        std::string s(lineBuff);
        if (len == 1 && !s.empty()) {
            values.push_back(lineBuff);
        }
    }
    fclose(fp);
    return true;
}

bool TextConfigFile::write(const std::string &fname, const std::vector<std::string> &values)
{
    FILE* fp = fopen(fname.c_str(), "w");
    if (!fp) {
        return false;
    }

    for (unsigned int i = 0; i < values.size(); ++i) {
        fprintf(fp, "%s\n", values[i].c_str());
    }
    fclose(fp);
    return true;
}



BinlogFileList::BinlogFileList(void)
{
}

BinlogFileList::~BinlogFileList(void)
{
}

int BinlogFileList::indexOfFileName(const std::string &fileName) const
{
    for (unsigned int i = 0; i < m_binlogFiles.size(); ++i) {
        if (m_binlogFiles[i] == fileName) {
            return i;
        }
    }
    return -1;
}

void BinlogFileList::loadFromFile(BinlogFileList *index, const std::string &fname)
{
    index->m_binlogFiles.clear();
    TextConfigFile::read(fname, index->m_binlogFiles);
}

void BinlogFileList::saveToFile(BinlogFileList *index, const std::string &fname)
{
    TextConfigFile::write(fname, index->m_binlogFiles);
}



Binlog::Binlog(void)
{
    m_fp = NULL;
    m_writtenSize = 0;
}

Binlog::~Binlog(void)
{
    close();
}

bool Binlog::open(const std::string &fname)
{
    close();

    FILE* fp = fopen(fname.c_str(), "a+");
    if (!fp) {
        Logger::log(Logger::Error, "Binlog::open: fopen() failed: %s", strerror(errno));
        return false;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        Logger::log(Logger::Error, "Binlog::open: fseek() failed: %s", strerror(errno));
        return false;
    }

    size_t fileSize = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        Logger::log(Logger::Error, "Binlog::open: fseek() failed: %s", strerror(errno));
        return false;
    }

    bool error = false;
    if (fileSize < sizeof(Header)) {
        //Write header
        Header header;
        header.handshake = Header::_handshake;
        if (fwrite(&header, sizeof(Header), 1, fp) != 1) {
            Logger::log(Logger::Error, "Binlog::open: write header failed: %s", strerror(errno));
            error = true;
        }
    } else {
        //Check header
        Header header;
        size_t readsize = fread(&header, sizeof(Header), 1, fp);
        if (readsize == 1) {
            if (!header.isValid()) {
                Logger::log(Logger::Error, "Binlog::open: file invalid");
                error = true;
            }
        } else {
            Logger::log(Logger::Error, "Binlog::open: read header failed: %s", strerror(errno));
            error = true;
        }
    }

    if (error) {
        fclose(fp);
        return false;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        Logger::log(Logger::Error, "Binlog::open: fseek() failed: %s", strerror(errno));
        return false;
    }

    m_fp = fp;
    m_fileName = fname;
    m_writtenSize = ftell(m_fp);
    return true;
}

void Binlog::sync(void)
{
    if (m_fp) {
        fflush(m_fp);
    }
}

void Binlog::close(void)
{
    if (m_fp) {
        fclose(m_fp);
        m_fileName = std::string();
        m_writtenSize = 0;
        m_fp = NULL;
    }
}

bool Binlog::appendSetRecord(const char *key, int klen, const char *value, int vlen)
{
    int writesize = sizeof(LogItem) + klen + vlen;
    LogItem item;
    item.item_size = writesize;
    item.type = LogItem::SET;
    item.key_size = klen;
    item.value_size = vlen;

    size_t result = fwrite(&item, sizeof(item), 1, m_fp);
    result = fwrite(key, klen, 1, m_fp);
    result = fwrite(value, vlen, 1, m_fp);

    m_writtenSize += writesize;
    return true;
}

bool Binlog::appendDelRecord(const char *key, int klen)
{
    int writesize = sizeof(LogItem) + klen;
    LogItem item;
    item.item_size = writesize;
    item.type = LogItem::DEL;
    item.key_size = klen;
    item.value_size = 0;

    size_t result = fwrite(&item, sizeof(item), 1, m_fp);
    result = fwrite(key, klen, 1, m_fp);

    m_writtenSize += writesize;
    return true;
}



BinlogParser::BinlogParser(void)
{
    m_base = NULL;
    m_size = 0;
    m_fd = -1;
}

BinlogParser::~BinlogParser(void)
{
    close();
}

bool BinlogParser::open(const std::string &fname)
{
    close();
#ifndef WIN32
    struct stat sbuf;
    int size;
    if (::stat(fname.c_str(), &sbuf) != 0) {
        Logger::log(Logger::Error, "BinlogParser::parse: stat(%s): %s", fname.c_str(), strerror(errno));
        return false;
    } else {
        size = sbuf.st_size;
    }

    int fd = ::open(fname.c_str(), O_RDONLY);
    if (fd < 0) {
        Logger::log(Logger::Error, "BinlogParser::parse: open: %s", strerror(errno));
        return false;
    }

    Binlog::Header header;
    if (::read(fd, (char*)&header, sizeof(header)) != sizeof(header)) {
        ::close(fd);
        Logger::log(Logger::Error, "BinlogParser::parse: read: %s", strerror(errno));
        return false;
    }

    if (!header.isValid()) {
        ::close(fd);
        Logger::log(Logger::Error, "BinlogParser::parse: file invalid");
        return false;
    }

    void* base = ::mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
        ::close(fd);
        Logger::log(Logger::Error, "BinlogParser::parse: mmap: %s", strerror(errno));
        return false;
    }
    m_base = (char*)base;
    m_size = size;
    m_fd = fd;
#else
    (void)fname;
#endif
    return true;
}

BinlogBufferReader BinlogParser::reader(void) const
{
    char* buff = m_base + sizeof(Binlog::Header);
    int size = m_size - sizeof(Binlog::Header);
    return BinlogBufferReader(buff, size);
}

void BinlogParser::close(void)
{
#ifndef WIN32
    if (m_base != NULL) {
        munmap(m_base, m_size);
        m_base = NULL;
        m_size = 0;
    }

    if (m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
#endif
}
