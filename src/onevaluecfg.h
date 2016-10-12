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

/*
    used to read xml config file
*/
#ifndef ONEVALUE_CONFIG
#define ONEVALUE_CONFIG
#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>

#include "tinyxml/tinyxml.h"
#include "tinyxml/tinystr.h"
#include "util/logger.h"
#include "redisproxy.h"

using namespace std;

typedef class COperateXml
{
public:
    COperateXml();
    ~COperateXml();

    bool xml_open(const char* xml_path);
    const TiXmlElement* get_rootElement();
public:
    // xml file operate pointer
    TiXmlDocument* m_docPointer;
private:
    TiXmlElement* m_rootElement;
private:
    COperateXml(const COperateXml&);
    COperateXml& operator =(const COperateXml&);
}COperateXml;


class CDbNode {
public:
    CDbNode() { hash_min = 0; hash_max = 0;}
    ~CDbNode(){}
    char db_name[256];
    int hash_min;
    int hash_max;
};
typedef vector<CDbNode> CDbNodeList;

// sync:0:Asynchronous   1:Synchronous
// compress:0: noet compress   1:compress
class COption {
public:
    COption();
    ~COption();
    bool sync() const {return m_sync;}
    bool compress() const {return m_compress;}
    int lruCacheSize()const {return m_lruCacheSize * 1024 * 1024;} // return bit
    int writeBufSize()const {return m_writeBufSize * 1024 * 1024;}
private:
    bool m_sync;
    bool m_compress;
    int m_lruCacheSize;
    int m_writeBufSize;
    friend class COneValueCfg;
};

class CBinLog {
public:
    CBinLog() { max_binlog_size = 0; _enabled = false;}
    unsigned int maxBinlogSize() {
        if (max_binlog_size <= 0) {
            return 1024 * 1024 * 2;
        }
        return max_binlog_size * 1024 * 1024;
    }
    bool enabled() { return _enabled; }
private:
    bool _enabled;
    int max_binlog_size; // bits;
    friend class COneValueCfg;
};


struct SMaster {
    SMaster(){
        memset(ip, '\0', sizeof(ip));
        port = 0;
        syncInterval = 5000;
    }
    char ip[256];
    int port;
    int syncInterval;
};

// read the config
class COneValueCfg
{
private:
    COneValueCfg();
    ~COneValueCfg();
public:
    static COneValueCfg* instance();
    bool loadCfg(const char* xml_path);

    int port() const{ return m_port;}
    int threadNum() const{ return m_threadNum;}
    int hashMax() const{ return m_hashMax;}
    bool topKeyEnable() const{ return m_topKeyEnable;}
    bool guard() const{return m_guard;}
    bool daemonize() const{return m_daemonize;}
    const char* logFile() const{ return m_logFile; }
    const char* workDir() const{ return m_workDir; }
    const char* pidFile() const{ return m_pidFile; }
    const char* unixSocketFile() const{ return m_unixSocketFile; }
    int dbCnt() { return m_dbNodes.size();}
    CDbNode* dbIndex(int num) { return &m_dbNodes[num];}
    COption* dbOption() { return &m_option;}

    CBinLog* binlog() {return &m_binlog;}
    SMaster* master() {return &m_master;}
private:
    void getRootAttr(const TiXmlElement* pRootNode);
    void getDbOption(const TiXmlAttribute* pRootNode);
    void getBinLog(const TiXmlAttribute* pEle);
    void getMaster(const TiXmlAttribute* pEle);
private:
    COperateXml*     m_operateXmlPointer;
    int              m_threadNum;
    int              m_port;
    int              m_hashMax;
    bool             m_topKeyEnable;
    bool             m_daemonize;
    bool             m_guard;
    char             m_logFile[512];
    char             m_workDir[512];
    char             m_pidFile[512];
    char             m_unixSocketFile[512];
    CDbNodeList      m_dbNodes;
    COption          m_option;
    CBinLog          m_binlog;
    SMaster          m_master;
private:
    COneValueCfg(const COneValueCfg&);
    COneValueCfg& operator =(const COneValueCfg&);
};

// check the cfg is valid or not
class COneValueCfgChecker
{
public:
    enum {ONEVALUE_HASH_MAX = 1024};
    COneValueCfgChecker(){}
    ~COneValueCfgChecker(){}
    static bool isValid(COneValueCfg* pCfg, const char*& err);
};

#endif
