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

#ifndef SYNC_H
#define SYNC_H

#include <stdlib.h>
#include <string>
#include "redisproxy.h"
#include "binlog.h"
#include "util/thread.h"
using namespace std;


class Sync : public Thread
{
public:
    #define RECVTIMEOUT 5000
    Sync(RedisProxy* proxy, const char* master, int port);
    ~Sync();
    void setSyncInterval(int t);
    void setReconnectInterval(int t);
protected:
    virtual void run(void);
private:
    bool _onSetCommand(Binlog::LogItem* item, LeveldbCluster* db);
    bool _onDelCommand(Binlog::LogItem* item, LeveldbCluster* db);
    void repairConnect(void);
private:
    int                      m_syncInterval;
    int                      m_reconnectInterval;
    HostAddress              m_masterAddr;
    TcpSocket                m_socket;

    RedisProxy*              m_proxy;

    string                   m_slaveIndexFileName;
    std::vector<std::string> m_masterSyncInfo;
private:
    Sync(const Sync&);
    Sync& operator =(const Sync& rhs);
};


#endif


