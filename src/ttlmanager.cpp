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
#include <stdio.h>
#include "util/logger.h"
#include "t_redis.h"
#include "ttlmanager.h"


class TTLThread : public Thread
{
public:
    TTLThread(TTLManager* ttl);
    ~TTLThread();
    virtual void run();
    void setSleepTime(unsigned int t);
private:
    TTLManager*  m_ttlManager;
    unsigned int m_sleepTime;
};


TTLThread::TTLThread(TTLManager* ttlMa)
{
    m_ttlManager = ttlMa;
    m_sleepTime  = 500;
}

TTLThread::~TTLThread()
{
}


void TTLThread::setSleepTime(unsigned int t)
{
    m_sleepTime = t;
}

void TTLThread::run()
{
    LeveldbCluster* dbClu = m_ttlManager->leveldbCluster();
    while (true) {
        for (int i = 0; i < dbClu->databaseCount(); ++i){
            Leveldb* db = dbClu->database(i);
            IOBuffer buf;
            XObject  key;
            ExpireKey::makeExpireKey(buf, key);
            XObject _key(buf.data(), buf.size());

            LeveldbIterator it;
            db->initIterator(it);
            it.seek(_key);
            unsigned int now = (unsigned int)time(NULL);
            int loop = 0;
            while (it.isValid()) {
                XObject expire_key = it.key();
                ExpireKey* pExpireKey = (ExpireKey*)expire_key.data;
                if (pExpireKey->type != T_Ttl) {
                    break;
                }

                XObject val = it.value();
                unsigned int expireTime = *((unsigned int*)val.data);
                if (now >= expireTime) {
                    dbClu->remove(expire_key);
                    dbClu->remove(XObject(pExpireKey->keyBuf(), pExpireKey->keyLen));
                }
                it.next();
                if (++loop = 100000) {
                    loop = 0;
                    now = (unsigned int)time(NULL);
                }
            }
        }
        Thread::sleep(m_sleepTime);
    }
}




TTLManager::TTLManager(LeveldbCluster* dbCluster)
{
    m_dbCluster = dbCluster;
    m_ttlThread = new TTLThread(this);
}

TTLManager::~TTLManager()
{
    delete m_ttlThread;
}


bool TTLManager::setExpire(const XObject& key, unsigned int t)
{
    IOBuffer buf;
    ExpireKey::makeExpireKey(buf, key);
    XObject _key(buf.data(), buf.size());

    unsigned int exp = (unsigned int)(time(NULL) + t);
    XObject _value((char*)&exp, sizeof(unsigned int));

    return m_dbCluster->setValue(_key, _value);
}

void TTLManager::start()
{
    m_ttlThread->start();
}

void TTLManager::stop()
{
    m_ttlThread->terminate();
}



void ExpireKey::makeExpireKey(IOBuffer& buf, const XObject& key)
{
    ExpireKey ttlKey;
    ttlKey.type = T_Ttl;
    ttlKey.keyLen = key.len;

    buf.append((char*)&ttlKey, sizeof(ExpireKey));
    buf.append(key.data, key.len);
}


