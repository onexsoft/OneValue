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

#include "onevaluecfg.h"

#ifdef WIN32
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif


COperateXml::COperateXml() {
    m_docPointer = new TiXmlDocument;
}

COperateXml::~COperateXml() {
    delete m_docPointer;
}

bool COperateXml::xml_open(const char* xml_path) {
    if (!m_docPointer->LoadFile(xml_path)) {
        return false;
    }
    m_rootElement = m_docPointer->RootElement();
    return true;
}

const TiXmlElement* COperateXml::get_rootElement() {
    return m_rootElement;
}

COneValueCfg::COneValueCfg() {
    m_operateXmlPointer = new COperateXml;
    m_hashMax = 0;
    m_threadNum = 8;
    m_port = 0;
    memset(m_logFile, '\0', sizeof(m_logFile));
    memset(m_workDir, '\0', sizeof(m_workDir));
    memset(m_pidFile, '\0', sizeof(m_pidFile));
    memset(m_unixSocketFile, '\0', sizeof(m_unixSocketFile));
    m_daemonize = false;
    m_guard = false;
    m_topKeyEnable = false;
}


COneValueCfg* COneValueCfg::instance() {
    static COneValueCfg* cfg = NULL;
    if (NULL == cfg) {
        cfg = new COneValueCfg;
    }
    return cfg;
}


COneValueCfg::~COneValueCfg() {
    if(NULL != m_operateXmlPointer) {
        delete m_operateXmlPointer;
        m_operateXmlPointer = NULL;
    }
}


void COneValueCfg::getRootAttr(const TiXmlElement* pRootNode) {
    TiXmlAttribute *addrAttr = (TiXmlAttribute *)pRootNode->FirstAttribute();
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (value == NULL) value = "";
        if (0 == strcasecmp(name, "thread_num")) {
            m_threadNum = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "port")) {
            m_port = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "hash_value_max")) {
            m_hashMax = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "daemonize")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                m_daemonize = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "guard")) {
            if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0) {
                m_guard = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "log_file")) {
            strcpy(m_logFile, value);
            continue;
        }
        if (0 == strcasecmp(name, "work_dir")) {
            strcpy(m_workDir, value);
            continue;
        }
        if (0 == strcasecmp(name, "pid_file")) {
            strcpy(m_pidFile, value);
            continue;
        }
        if (0 == strcasecmp(name, "unix_socket_file")) {
            strcpy(m_unixSocketFile, value);
            continue;
        }
    }
}


COption::COption() {
    m_sync = false;
    m_compress = false;
    m_lruCacheSize = 0;
    m_writeBufSize = 4;
    m_blocksize = 16;
    m_maxfilesize = 16;
}

COption::~COption() {}

void COneValueCfg::getDbOption(const TiXmlAttribute* addrAttr) {
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (0 == strcasecmp(name, "sync")) {
            if (atoi(value) > 0) {
                m_option.m_sync = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "compress")) {
            if (atoi(value) > 0) {
                m_option.m_compress = true;
            }
            continue;
        }
        if (0 == strcasecmp(name, "block_size")) {
            if (atoi(value) > 0) {
                m_option.m_blocksize = atoi(value);
            }
            continue;
        }
        if (0 == strcasecmp(name, "max_file_size")) {
            if (atoi(value) > 0) {
                m_option.m_maxfilesize = atoi(value);
            }
            continue;
        }
        if (0 == strcasecmp(name, "lru_cache_size")) {
            m_option.m_lruCacheSize = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "write_buf_size")) {
            m_option.m_writeBufSize = atoi(value);
            continue;
        }
    }
}


void COneValueCfg::getBinLog(const TiXmlAttribute* addrAttr) {
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        int iValue = atoi(value);
        if (0 == strcasecmp(name, "max_binlog_size")) {
            m_binlog.max_binlog_size = iValue;
            continue;
        }
        if (0 == strcasecmp(name, "enabled")) {
            if (iValue > 0) {
                m_binlog._enabled = true;
            }
        }
    }
}

void COneValueCfg::getMaster(const TiXmlAttribute* addrAttr) {
    for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
        const char* name = addrAttr->Name();
        const char* value = addrAttr->Value();
        if (0 == strcasecmp(name, "ip")) {
            strcpy(m_master.ip, value);
            continue;
        }
        if (0 == strcasecmp(name, "port")) {
            m_master.port = atoi(value);
            continue;
        }
        if (0 == strcasecmp(name, "sync_interval")) {
            m_master.syncInterval= atoi(value);
            continue;
        }
    }
}


bool COneValueCfg::loadCfg(const char* file) {
    if (!m_operateXmlPointer->xml_open(file)) return false;

    const TiXmlElement* pRootNode = m_operateXmlPointer->get_rootElement();
    getRootAttr(pRootNode);
    TiXmlElement* pNode = (TiXmlElement*)pRootNode->FirstChildElement();
    for (; pNode != NULL; pNode = pNode->NextSiblingElement()) {
        if (0 == strcasecmp(pNode->Value(), "top_key")) {
            TiXmlAttribute *addrAttr = (TiXmlAttribute *)pNode->FirstAttribute();
            for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
                const char* name = addrAttr->Name();
                const char* value = addrAttr->Value();
                if (0 == strcasecmp(name, "enable")) {
                    if(strcasecmp(value, "0") != 0 && strcasecmp(value, "") != 0 ) {
                        m_topKeyEnable = true;
                    }
                }
            }
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "db_option")) {
            TiXmlAttribute *addrAttr = (TiXmlAttribute*)pNode->FirstAttribute();
            getDbOption(addrAttr);
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "master")) {
            TiXmlAttribute *addrAttr = (TiXmlAttribute*)pNode->FirstAttribute();
            getMaster(addrAttr);
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "binlog")) {
            TiXmlAttribute *addrAttr = (TiXmlAttribute*)pNode->FirstAttribute();
            getBinLog(addrAttr);
            continue;
        }

        if (0 == strcasecmp(pNode->Value(), "db_node")) {
            CDbNode dbNode;
            TiXmlAttribute *addrAttr = (TiXmlAttribute*)pNode->FirstAttribute();
            for (; addrAttr != NULL; addrAttr = addrAttr->Next()) {
                const char* name = addrAttr->Name();
                const char* value = addrAttr->Value();
                if (0 == strcasecmp(name, "name")) {
                    strcpy(dbNode.db_name, value);
                    continue;
                }
                if (0 == strcasecmp(name, "hash_min")) {
                    dbNode.hash_min = atoi(value);
                    continue;
                }
                if (0 == strcasecmp(name, "hash_max")) {
                    dbNode.hash_max = atoi(value);
                }
            }
            m_dbNodes.push_back(dbNode);
        }
    }

    return true;
}

bool COneValueCfgChecker::isValid(COneValueCfg* pCfg, const char*& errMsg)
{
    const int hash_value_max = pCfg->hashMax();
    if (hash_value_max > ONEVALUE_HASH_MAX) {
        errMsg = "max hash_value_max is larger than 1024";
        return false;
    }
    bool barray[ONEVALUE_HASH_MAX] = {0};
    map<string, int> nameMap;
    for (int i = 0; i < pCfg->dbCnt(); ++i) {
        const CDbNode* dbNode = pCfg->dbIndex(i);
        if (strlen(dbNode->db_name) == 0) {
            errMsg = "dbNode name is empty";
            return false;
        }
        if (nameMap.find(string(dbNode->db_name)) == nameMap.end()) {
            nameMap.insert(map<string,int>::value_type(string(dbNode->db_name), 1));
        } else {
            errMsg = "exist same dbNode name";
            return false;
        }

        if (dbNode->hash_min > dbNode->hash_max) {
            errMsg = "hash_min > hash_max";
            return false;
        }
        for (int j = dbNode->hash_min; j <= dbNode->hash_max; ++j) {
            if (j < 0 || j > ONEVALUE_HASH_MAX) {
                errMsg = "hash value is invalid";
                return false;
            }
            if (j > hash_value_max) {
                errMsg = "hash value is out of range";
                return false;
            }

            if (barray[j]) {
                errMsg = "range error";
                return false;
            }
            barray[j] = true;
        }
    }
    for (int i = 0; i < hash_value_max; ++i) {
        if (!barray[i]) {
            errMsg = "parameter is not complete";
            return false;
        }
    }

    COption* option = pCfg->dbOption();
    if (option->writeBufSize() < 0) {
        errMsg = "write_buf_size < 0";
        return false;
    }

    return true;
}
