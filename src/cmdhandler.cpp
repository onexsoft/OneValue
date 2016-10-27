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

#include "util/logger.h"
#include "command.h"
#include "leveldb.h"
#include "t_redis.h"
#include "t_hyperloglog.h"
#include "dbcopy.h"
#include "ttlmanager.h"
#include "sync.h"
#include "cmdhandler.h"

class StringMutex
{
public:
    enum { Size = 128 };
    StringMutex(void) {}
    ~StringMutex(void) {}

    void lock(const XObject& key) {
        unsigned int index = hashForBytes(key.data, key.len) % Size;
        m_mutex[index].lock();
    }

    void unlock(const XObject& key) {
        unsigned int index = hashForBytes(key.data, key.len) % Size;
        m_mutex[index].unlock();
    }

private:
    Mutex m_mutex[Size];
};

static StringMutex string_mutex;

XObject makeStringKey(const char* rawkey, int rawkey_size, std::string& store)
{
    short type = T_KV;
    store.append((char*)&type, sizeof(type));
    store.append(rawkey, rawkey_size);
    return XObject(store.data(), store.size());
}

void onRawSetCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3 || r.tokenCount > 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    LeveldbCluster::WriteOption opt;
    if (r.tokenCount == 4) {
        opt.mapping_key = XObject(r.tokens[3].s, r.tokens[3].len);
    }

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key(r.tokens[1].s, r.tokens[1].len);
    XObject value(r.tokens[2].s, r.tokens[2].len);
    db->setValue(key, value, opt);
    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onAppendCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();

    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    XObject appendValue(r.tokens[2].s, r.tokens[2].len);

    string_mutex.lock(key);
    std::string val;
    if (!db->value(key, val)) {
        db->setValue(key, appendValue);
        packet->sendBuff.appendFormatString(":%d\r\n", appendValue.len);
    } else {
        val.append(appendValue.data, appendValue.len);
        db->setValue(key, XObject(val.data(), val.size()));
        packet->sendBuff.appendFormatString(":%d\r\n", val.length());
    }
    string_mutex.unlock(key);

    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onDecrCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();

    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    string_mutex.lock(key);

    std::string val;
    db->value(key, val);

    if (!TRedisHelper::isInteger(val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int oldValue = atoi(val.c_str());

    char newValue[32];
    int n = sprintf(newValue, "%d", --oldValue);
    db->setValue(key, XObject(newValue, n));
    string_mutex.unlock(key);

    packet->sendBuff.appendFormatString(":%d\r\n", oldValue);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onDecrByCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string strbyvalue(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isInteger(strbyvalue)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    string_mutex.lock(key);

    std::string val;
    db->value(key, val);

    if (!TRedisHelper::isInteger(val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int oldValue = atoi(val.c_str());
    int byvalue = atoi(strbyvalue.c_str());

    char newValue[32];
    int n = sprintf(newValue, "%d", oldValue - byvalue);
    db->setValue(key, XObject(newValue, n));
    string_mutex.unlock(key);

    packet->sendBuff.appendFormatString(":%d\r\n", oldValue - byvalue);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


bool transformStringIndex(int& start, int& stop, int maxsize)
{
    if (start < 0) {
        start = maxsize + start;
        if (start < 0) {
            start = 0;
        }
    }
    if (stop < 0) {
        stop = maxsize + stop;
        if (stop < 0) {
            stop = 0;
        }
    }
    if (start > (maxsize - 1)) {
        start = maxsize - 1;
    }
    if (stop >= (maxsize - 1)) {
        stop = maxsize - 1;
    }

    if (start < 0 || stop < 0) {
        return false;
    }
    return true;
}

void onGetRangeCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string str_start(r.tokens[2].s, r.tokens[2].len);
    std::string str_stop(r.tokens[3].s, r.tokens[3].len);

    if (!TRedisHelper::isInteger(str_start) || !TRedisHelper::isInteger(str_stop)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    std::string val;
    db->value(key, val);

    int start = atoi(str_start.c_str());
    int stop = atoi(str_stop.c_str());
    if (!transformStringIndex(start, stop, val.length())) {
        packet->sendBuff.append("$0\r\n\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    if (stop - start < 0) {
        packet->sendBuff.append("$0\r\n\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int len = (stop - start) + 1;
    packet->sendBuff.appendFormatString("$%d\r\n", len);
    packet->sendBuff.append(val.data() + start, len);
    packet->sendBuff.append("\r\n", 2);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onGetSetCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    XObject newValue(r.tokens[2].s, r.tokens[2].len);

    string_mutex.lock(key);
    std::string oldValue;
    if (!db->value(key, oldValue)) {
        packet->sendBuff.append("$-1\r\n");
    } else {
        packet->sendBuff.appendFormatString("$%d\r\n%s\r\n", oldValue.length(), oldValue.c_str());
    }
    db->setValue(key, newValue);
    string_mutex.unlock(key);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onIncrCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();

    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    string_mutex.lock(key);
    std::string val;
    db->value(key, val);

    if (!TRedisHelper::isInteger(val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int oldValue = atoi(val.c_str());

    char newValue[32];
    int n = sprintf(newValue, "%d", ++oldValue);
    db->setValue(key, XObject(newValue, n));
    string_mutex.unlock(key);

    packet->sendBuff.appendFormatString(":%d\r\n", oldValue);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onIncrByCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string strbyvalue(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isInteger(strbyvalue)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    string_mutex.lock(key);
    std::string val;
    db->value(key, val);

    if (!TRedisHelper::isInteger(val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int oldValue = atoi(val.c_str());
    int byvalue = atoi(strbyvalue.c_str());

    char newValue[32];
    int n = sprintf(newValue, "%d", oldValue + byvalue);
    db->setValue(key, XObject(newValue, n));
    string_mutex.unlock(key);

    packet->sendBuff.appendFormatString(":%d\r\n", oldValue + byvalue);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onIncrByFloatCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string strbyvalue(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isDouble(strbyvalue)) {
        packet->sendBuff.append("-ERR value is not a valid float\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    string_mutex.lock(key);
    std::string val;
    db->value(key, val);

    if (!TRedisHelper::isDouble(val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("-ERR value is not a valid float\r\n");
        return;
    }

    double oldValue = atof(val.c_str());
    double byvalue = atof(strbyvalue.c_str());

    char newValue[32];
    int n = TRedisHelper::doubleToString(newValue, oldValue + byvalue, true);
    db->setValue(key, XObject(newValue, n));
    string_mutex.unlock(key);

    packet->sendBuff.appendFormatString("$%d\r\n%s\r\n", n, newValue);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onMGetCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    packet->sendBuff.appendFormatString("*%d\r\n", r.tokenCount - 1);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    for (int i = 1; i < r.tokenCount; ++i) {
        std::string store;
        std::string value;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
        if (db->value(key, value)) {
            packet->sendBuff.appendFormatString("$%d\r\n%s\r\n", value.length(), value.c_str());
        } else {
            packet->sendBuff.append("$-1\r\n");
        }
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onMSetCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3 || ((r.tokenCount - 1) % 2) != 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    for (int i = 1; i < r.tokenCount; i += 2) {
        std::string store;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
        XObject value(r.tokens[i+1].s, r.tokens[i+1].len);
        db->setValue(key, value);
    }
    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onExistsCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    std::string value;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    if (db->value(key, value)) {
        packet->sendBuff.appendFormatString(":1\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    packet->sendBuff.appendFormatString(":0\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onMSetNXCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3 || ((r.tokenCount - 1) % 2) != 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    for (int i = 1; i < r.tokenCount; i += 2) {
        std::string store;
        std::string value;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
        if (db->value(key, value)) {
            packet->sendBuff.appendFormatString(":0\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
    }

    for (int i = 1; i < r.tokenCount; i += 2) {
        std::string store;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
        XObject value(r.tokens[i+1].s, r.tokens[i+1].len);
        db->setValue(key, value);
    }
    packet->sendBuff.append(":1\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onPSetEXCommand(ClientPacket*, void *)
{
}


void onSetNXCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string val;
    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    string_mutex.lock(key);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    if (db->value(key, val)) {
        string_mutex.unlock(key);
        packet->sendBuff.append("$-1\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    XObject value(r.tokens[2].s, r.tokens[2].len);
    if (db->setValue(key, value)) {
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Unknown error\r\n");
    }
    string_mutex.unlock(key);
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSetEXCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string str_expire(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isInteger(str_expire)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    int expire = atoi(str_expire.c_str());
    if (expire <= 0) {
        packet->sendBuff.append("-ERR invalid expire time in set\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();

    string_mutex.lock(key);
    XObject value(r.tokens[3].s, r.tokens[3].len);
    if (db->setValue(key, value)) {
        db->ttlManager()->setExpire(key, expire);
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Unknown error\r\n");
    }
    string_mutex.unlock(key);
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSetRangeCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string offset(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isInteger(offset)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    int offset_ = atoi(offset.c_str());
    if (offset_ < 0) {
        packet->sendBuff.append("-ERR offset is out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    string_mutex.lock(key);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    std::string val;
    std::string replace(r.tokens[3].s, r.tokens[3].len);
    if (db->value(key, val)) {
        if ((unsigned int)offset_ > val.length()) {
            int t = offset_ - val.length();
            val.append(t, '\x00');
            val.append(replace.data(), replace.size());
        }else if ((unsigned int)offset_ == val.length()) {
            val.append(replace.data(), replace.size());
        } else if ((unsigned int)offset_ < val.length()) {
            val.replace(offset_, replace.length(), replace.data());
        }
    } else {
        val.assign(offset_ + 1, '\x00');
        val += replace;
    }
    db->setValue(key, XObject(val.data(), val.size()));
    string_mutex.unlock(key);
    packet->sendBuff.appendFormatString(":%d\r\n", val.length());
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onStrlenCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string store;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);

    std::string value;
    db->value(key, value);
    packet->sendBuff.appendFormatString(":%d\r\n", value.length());
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onGetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string val;
    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    if (db->value(key, val)) {
        packet->sendBuff.appendFormatString("$%d\r\n", val.size());
        packet->sendBuff.append(val.data(), val.size());
        packet->sendBuff.append("\r\n");
    } else {
        packet->sendBuff.append("$-1\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    int expire = -1;
    for (int i = 3; i < r.tokenCount; ++i) {
        if (strncasecmp(r.tokens[i].s, "ex", 2) == 0) {
            if (i + 1 >= r.tokenCount) {
                packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                return;
            }
            std::string str_expire(r.tokens[i+1].s, r.tokens[i+1].len);
            if (!TRedisHelper::isInteger(str_expire)) {
                packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
                return;
            }
            expire = atoi(str_expire.c_str());
            if (expire <= 0) {
                packet->sendBuff.append("-ERR invalid expire time in set\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
                return;
            }
            ++i;
        } else {
            packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
            return;
        }
    }

    std::string store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    XObject value(r.tokens[2].s, r.tokens[2].len);

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    if (db->setValue(key, value)) {
        if (expire != -1) {
            db->ttlManager()->setExpire(key, expire);
        }
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Unknown error\r\n");
    }

    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onDelCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    int succeed = 0;
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    for (int i = 1; i < r.tokenCount; ++i) {
        std::string store;
        std::string value;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
        if (db->value(key, value)) {
            db->remove(key);
            ++succeed;
        }
    }
    packet->sendBuff.appendFormatString(":%d\r\n", succeed);
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onFlushdbCommand(ClientPacket* packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 1) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    packet->proxy()->leveldbCluster()->clear();
    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onExpireCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string str_expire(r.tokens[2].s, r.tokens[2].len);
    if (!TRedisHelper::isInteger(str_expire)) {
        packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    int expire = atoi(str_expire.c_str());
    if (expire <= 0) {
        packet->sendBuff.append("-ERR invalid expire time in set\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    IOBuffer keybuff;
    short type = T_KV;
    keybuff.append((char*)&type, sizeof(type));
    keybuff.append(r.tokens[1].s, r.tokens[1].len);

    XObject key(keybuff.data(), keybuff.size());

    packet->proxy()->leveldbCluster()->ttlManager()->setExpire(key, expire);
    packet->sendBuff.append(":1\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onPFAddCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if(r.tokenCount < 3){
	packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
	return;
    }
    
    std::string val, str_register, store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    
    string_mutex.lock(key);
    if(db->value(key, val)){
	str_register = val;
    }else{
	str_register = "";
    }    
    
    THyperLogLog log(10, str_register);
    
    std::string result;
    for(int i = 2; i < r.tokenCount; ++i){
	result = log.Add(r.tokens[i].s, r.tokens[i].len);
    }
    XObject value(result.data(), result.size());

    if (db->setValue(key, value)) {
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Unknown error\r\n");
    }
    string_mutex.unlock(key);

    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onPFCountCommand(ClientPacket* packet, void*)
{
   RedisProtoParseResult& r = packet->recvParseResult;
   if(r.tokenCount < 2){
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    
    std::string val, str_register, store;
    XObject key = makeStringKey(r.tokens[1].s, r.tokens[1].len, store);
    if(db->value(key, val)){
        str_register = val;
    }else{
	packet->sendBuff.append("$-1\r\n");
    }
    THyperLogLog first_log(10, str_register);
    
    for(int i = 2; i < r.tokenCount; ++i)
    {
	std::string val, str_register, store;
        XObject key = makeStringKey(r.tokens[i].s, r.tokens[i].len, store);
    	if(db->value(key, val)){
            str_register = val;
	}else{
	    packet->sendBuff.append("$-1\r\n");
	}
	THyperLogLog log(10, str_register);
	first_log.Merge(log);
    }
    std::string estimate = to_string(first_log.Estimate());
    packet->sendBuff.appendFormatString("$%d\r\n", estimate.size());
    packet->sendBuff.append(estimate.data(), estimate.size());
    packet->sendBuff.append("\r\n");
    
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onPFMergeCommand(ClientPacket * packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if(r.tokenCount < 3){
	packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
	return;
    }

    LeveldbCluster* db = packet->proxy()->leveldbCluster();
    
    std::string store1, str_register1, val1;
    XObject key1 = makeStringKey(r.tokens[1].s, r.tokens[1].len, store1);
  
    string_mutex.lock(key1);
    if(db->value(key1, val1)){
	str_register1 = val1;
    }else{
	// TODO: ERROR key does not exists
	str_register1 = "";
    }
    THyperLogLog log1(10, str_register1);
   
    std::string result; 
    for(int i = 2; i < r.tokenCount; ++i){
	std::string store2, str_register2, val2;
    	XObject key2 = makeStringKey(r.tokens[i].s, r.tokens[i].len, store2);
    	if(db->value(key2, val2)){
		str_register2 = val2;
    	}else{
		// TODO: ERROR key does not exists
		str_register2 = "";
    	}

    	THyperLogLog log2(10, str_register2);
	result = log1.Merge(log2);
    }
   
    XObject value(result.data(), result.size());
    if (db->setValue(key1, value)) {
    	packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Unknown error\r\n");
    }
    string_mutex.unlock(key1);
    
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onPingCommand(ClientPacket* packet, void*)
{
    packet->sendBuff.append("+PONG\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onShowCommand(ClientPacket* packet, void*)
{
    IOBuffer& reply = packet->sendBuff;
    reply.append("+ *** Support the command ***\n");
    reply.append("[Redis Command]\n");

    std::string redis;
    std::string onevalue;
    const std::list<RedisCommand*>& seq = RedisCommandTable::instance()->commands();
    std::list<RedisCommand*>::const_iterator it = seq.cbegin();
    for (; it != seq.cend(); ++it) {
        RedisCommand* cmd = *it;
        if (cmd->type >= 0 && cmd->type < RedisCommand::CMD_COUNT) {
            redis.append(cmd->name);
            redis.append(" ");
        } else if (cmd->type == RedisCommand::PrivType) {
        } else {
            onevalue.append(cmd->name);
            onevalue.append(" ");
        }
    }

    reply.append(redis.data(), redis.length());
    reply.append("\n\n");
    reply.append("[OneValue Command]\n");
    reply.append(onevalue.data(), onevalue.length());
    reply.append("\n\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSyncCommand(ClientPacket* packet, void* arg)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    char fileName[256] = {0};
    int lastUpdatePos = 0;
    char _lastUpdatePos[64] = {0};
    strncpy(fileName, r.tokens[1].s, r.tokens[1].len);
    strncpy(_lastUpdatePos, r.tokens[2].s, r.tokens[2].len);
    lastUpdatePos = atoi(_lastUpdatePos);

    IOBuffer& reply = packet->sendBuff;
    reply.reserve(BinlogSyncStream::MaxStreamSize);
    RedisProxy* proxy = (RedisProxy*)arg;
    LeveldbCluster* db = proxy->leveldbCluster();
    Binlog* curBinlog = db->currentBinlog();
    BinlogFileList* flist = db->binlogFileList();

    BinlogSyncStream stream;
    reply.append((char*)&stream, sizeof(BinlogSyncStream));

    int count = 0;
    int firstPos = lastUpdatePos;
    int err = BinlogSyncStream::NoError;
    std::string errMsg;
    std::string lastFile = fileName;

    if (!flist->isEmpty()) {
        if (lastFile[0] == ' ') {
            firstPos = -1;
            lastFile = flist->fileName(0);
        }

        int index = flist->indexOfFileName(lastFile);
        if (index == -1) {
            err = BinlogSyncStream::InvalidFileName;
            errMsg = "invalid binlog file name";
        } else {
            for (; index < flist->fileCount(); ++index) {
                std::string s = flist->fileName(index);
                std::string fname = db->binlogFileName(s);

                BinlogParser parser;
                bool ok = false;
                if (fname == curBinlog->fileName()) {
                    db->lockCurrentBinlogFile();
                    curBinlog->sync();
                    ok = parser.open(fname);
                    db->unlockCurrentBinlogFile();
                } else {
                    ok = parser.open(fname);
                }

                if (!ok) {
                    break;
                }

                int pos = -1;
                bool isBreak = false;
                BinlogBufferReader reader = parser.reader();
                Binlog::LogItem* item = reader.firstItem();
                for (; item != NULL; item = reader.nextItem(item)) {
                    if (pos >= firstPos) {
                        reply.append((char*)item, item->item_size);
                        ++count;
                    }
                    ++pos;
                    if (reply.size() >= BinlogSyncStream::MaxStreamSize) {
                        isBreak = true;
                        break;
                    }
                }
                firstPos = -1;
                lastFile = s;
                lastUpdatePos = pos;
                if (isBreak) {
                    break;
                }
            }
        }
    }
    reply.append("\r\n", 2);
    //Logger::log(Logger::Message, "SYNCPROC: lastUpdatePos=%d lastFile=%s", lastUpdatePos, lastFile.c_str());
    BinlogSyncStream* pStream = (BinlogSyncStream*)reply.data();
    pStream->ch = '+';
    pStream->streamSize = reply.size();
    pStream->error = err;
    strcpy(pStream->errorMsg, errMsg.c_str());
    strcpy(pStream->srcFileName, lastFile.c_str());
    pStream->lastUpdatePos = lastUpdatePos;
    pStream->logItemCount = count;
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onCopyCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->sendBuff.append("0");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    std::string str_port(r.tokens[1].s, r.tokens[1].len);
    int port = atoi(str_port.c_str());

    RedisProxy* proxy = packet->proxy();

    DBCopy copy;
    HostAddress sender(packet->clientAddress.ip(), port);

    if (copy.start(proxy->leveldbCluster(), sender)) {
        packet->sendBuff.append("1");
    } else {
        packet->sendBuff.append("0");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSyncFromCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string str_host(r.tokens[1].s, r.tokens[1].len);
    std::string str_port(r.tokens[2].s, r.tokens[2].len);
    int port = atoi(str_port.c_str());

    TcpSocket sock = TcpSocket::createTcpSocket();
    if (!sock.connect(HostAddress(str_host.c_str(), port))) {
        sock.close();
        packet->sendBuff.appendFormatString("-ERR Can't connect to host '%s:%d'\r\n", str_host.c_str(), port);
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    char portbuf[32];
    char sendbuf[128];
    RedisProxy* proxy = packet->proxy();
    Sync* syncThread = proxy->syncThread();

    int n = sprintf(portbuf, "%d", proxy->address().port());
    sprintf(sendbuf, "*2\r\n$6\r\n__copy\r\n$%d\r\n%s\r\n", n, portbuf);
    if (sock.send(sendbuf, strlen(sendbuf)) <= 0) {
        sock.close();
        packet->sendBuff.append("-ERR Request the host failed\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    //Stop sync thread
    if (syncThread) {
        syncThread->terminate();
    }

    char ch;
    sock.recv(&ch, 1);
    if (ch == '1') {
        packet->sendBuff.append("+OK\r\n");
    } else {
        packet->sendBuff.append("-ERR Copy failed\r\n");
    }

    sock.close();

    //Restart sync thread
    if (syncThread) {
        syncThread->start();
    }

    packet->setFinishedState(ClientPacket::RequestFinished);
}
