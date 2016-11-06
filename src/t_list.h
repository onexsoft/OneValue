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

#ifndef T_LIST_H
#define T_LIST_H

#include "util/iobuffer.h"
#include "t_redis.h"
#include "leveldb.h"

struct ListKey
{
    short type;
    short namelen;

    char* namebuff(void) const { return (char*)(this+1); }
    std::string name(void) const
    { return std::string(namebuff(), namelen); }

    static void makeListKey(const std::string& name, std::string& s)
    {
        ListKey key;
        key.type = T_List;
        key.namelen = name.size();
        s.append((char*)&key, sizeof(key));
        s.append(name.data(), name.size());
    }
};

struct ListElementKey
{
    short type;
    short namelen;
    int elementId;

    char* namebuff(void) const { return (char*)(this+1); }
    std::string name(void) const
    { return std::string(namebuff(), namelen); }

    static void makeListElementKey(const std::string& listname, int elementId, std::string& s)
    {
        ListElementKey key;
        key.type = T_ListElement;
        key.namelen = listname.size();
        key.elementId = elementId;
        s.append((char*)&key, sizeof(key));
        s.append(listname.data(), listname.size());
    }
};

struct ListValueBuffer
{
    int left_pos;
    int right_pos;
};

class TList
{
public:
    TList(LeveldbCluster* db, const std::string& name);
    ~TList(void);

    bool lindex(int index, std::string* value);
    int llen(void);
    bool lpop(std::string* value);
    int lpush(const std::string& value);
    int lpushx(const std::string& value);
    bool lrange(int start, int stop, stringlist* result);
    bool lset(int index, const std::string& value);
    bool ltrim(int start, int stop);
    bool rpop(std::string* value);
    int rpush(const std::string& value);
    int rpushx(const std::string& value);
    void lclear(void);

    const std::string& lastError(void) const { return m_lastError; }

protected:
    void setLastError(const std::string& s) { m_lastError = s; }

private:
    std::string m_lastError;
    LeveldbCluster* m_db;
    const std::string m_listname;
};

#endif
