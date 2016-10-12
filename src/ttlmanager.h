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



