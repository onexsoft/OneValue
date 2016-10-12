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

#include <set>
#include <algorithm>

#include "redisproxy.h"
#include "t_zset.h"
#include "t_hash.h"
#include "zsetcmdhandler.h"

//SET
void onSAddCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    int succNum = 0;
    for (int i = 2; i < tokenConut; ++i) {
        std::string key(parseResult.tokens[i].s, parseResult.tokens[i].len);
        if (t_set.hset(key, key)) {
            ++succNum;
        }
    }

    packet->sendBuff.appendFormatString(":%d\r\n", succNum);
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSCardCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    packet->sendBuff.appendFormatString(":%d\r\n", t_set.hlen());
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSIsMemberCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[1].s, parseResult.tokens[1].len);
    std::string val(parseResult.tokens[2].s, parseResult.tokens[2].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    int num = 0;
    if (t_set.hexists(val)) {
        num = 1;
    }
    packet->sendBuff.appendFormatString(":%d\r\n", num);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSDiffCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    std::list<std::string> vals;
    std::list<std::string> tmps;
    t_set.hgetall(&vals, &tmps);
    if (vals.empty()) {
        packet->sendBuff.append("*0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    std::list<std::string> compareResultList;
    std::list<std::string>::iterator itV = compareResultList.begin();
    for (int i = 2; i < tokenConut; ++i) {
        compareResultList.clear();
        std::string compareKey(parseResult.tokens[i].s, parseResult.tokens[i].len);
        TSet t_set_Compare(packet->proxy()->leveldbCluster(), compareKey);
        std::list<std::string> comp_vals;
        std::list<std::string> comp_tmps;
        t_set_Compare.hgetall(&comp_vals, &comp_tmps);
        compareResultList.resize(vals.size());

        itV = set_difference(vals.begin(), vals.end(), comp_vals.begin(), \
            comp_vals.end(), compareResultList.begin());
        std::list<std::string>::iterator itTmp = compareResultList.begin();
        int sizeTmp = 0;
        while (itTmp != itV) {
            ++sizeTmp;
            ++itTmp;
        }
        compareResultList.resize(sizeTmp);
        if (compareResultList.empty()) {
            packet->sendBuff.append("*0\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        vals = compareResultList;
    }
    std::list<std::string>::const_iterator itList = compareResultList.begin();
    packet->sendBuff.appendFormatString("*%d\r\n", compareResultList.size());
    for (; itList != compareResultList.end(); ++itList) {
        packet->sendBuff.appendFormatString("$%d\r\n", itList->size());
        packet->sendBuff.append(itList->data(), itList->size());
        packet->sendBuff.append("\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSDiffStoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName_(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set_first(packet->proxy()->leveldbCluster(), setName_);
    t_set_first.hclear();

    std::string setName(parseResult.tokens[2].s, parseResult.tokens[2].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    std::list<std::string> vals;
    std::list<std::string> tmps;
    t_set.hgetall(&vals, &tmps);

    if (vals.empty()) {
        packet->sendBuff.append(":0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    std::list<std::string> compareResultList;
    if (tokenConut < 4) {
        compareResultList = vals;
    }
    std::list<std::string>::iterator itV = compareResultList.begin();
    for (int i = 3; i < tokenConut; ++i) {
        std::string compareKey(parseResult.tokens[i].s, parseResult.tokens[i].len);
        TSet t_set_Compare(packet->proxy()->leveldbCluster(), compareKey);
        std::list<std::string> comp_vals;
        std::list<std::string> comp_tmps;
        t_set_Compare.hgetall(&comp_vals, &comp_tmps);
        compareResultList.resize(vals.size());
        itV = set_difference(vals.begin(), vals.end(), comp_vals.begin(), \
            comp_vals.end(), compareResultList.begin());
        std::list<std::string>::iterator itTmp = compareResultList.begin();
        int sizeTmp = 0;
        while (itTmp != itV) {
            ++sizeTmp;
            ++itTmp;
        }
        compareResultList.resize(sizeTmp);
        if (compareResultList.empty()) {
            packet->sendBuff.append(":0\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        vals = compareResultList;
    }
    std::list<std::string>::const_iterator itList = compareResultList.begin();
    for (; itList != compareResultList.end(); ++itList) {
        t_set_first.hset(*itList, *itList);
    }
    packet->sendBuff.appendFormatString(":%d\r\n", compareResultList.size());
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSInterCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    std::list<std::string> vals;
    std::list<std::string> tmps;
    t_set.hgetall(&vals, &tmps);
    if (vals.empty()) {
        packet->sendBuff.append("*0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    std::list<std::string> compareResultList;
    std::list<std::string>::iterator itV = compareResultList.begin();
    for (int i = 2; i < tokenConut; ++i) {
        std::string compareKey(parseResult.tokens[i].s, parseResult.tokens[i].len);
        TSet t_set_Compare(packet->proxy()->leveldbCluster(), compareKey);
        std::list<std::string> comp_vals;
        std::list<std::string> comp_tmps;
        t_set_Compare.hgetall(&comp_vals, &comp_tmps);
        compareResultList.resize(vals.size() + comp_vals.size());

        itV = set_intersection(vals.begin(), vals.end(), comp_vals.begin(), \
            comp_vals.end(), compareResultList.begin());
        std::list<std::string>::iterator itTmp = compareResultList.begin();
        int sizeTmp = 0;
        while (itTmp != itV) {
            ++sizeTmp;
            ++itTmp;
        }
        compareResultList.resize(sizeTmp);
        if (compareResultList.empty()) {
            packet->sendBuff.append("*0\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        vals = compareResultList;
    }
    std::list<std::string>::const_iterator itList = compareResultList.begin();
    packet->sendBuff.appendFormatString("*%d\r\n", compareResultList.size());
    for (; itList != compareResultList.end(); ++itList) {
        packet->sendBuff.appendFormatString("$%d\r\n", itList->length());
        packet->sendBuff.append(itList->data(), itList->length());
        packet->sendBuff.append("\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSInterStoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string setName(parseResult.tokens[2].s, parseResult.tokens[2].len);
    TSet t_set(packet->proxy()->leveldbCluster(), setName);
    std::list<std::string> vals;
    std::list<std::string> tmps;
    t_set.hgetall(&vals, &tmps);
    std::string setName_(parseResult.tokens[1].s, parseResult.tokens[1].len);
    TSet t_set_first(packet->proxy()->leveldbCluster(), setName_);
    t_set_first.hclear();
    if (vals.empty()) {
        packet->sendBuff.append(":0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    std::list<std::string> compareResultList;
    if (tokenConut < 4) {
        compareResultList = vals;
    }
    std::list<std::string>::iterator itV = compareResultList.begin();
    for (int i = 3; i < tokenConut; ++i) {
        std::string compareKey(parseResult.tokens[i].s, parseResult.tokens[i].len);
        TSet t_set_Compare(packet->proxy()->leveldbCluster(), compareKey);
        std::list<std::string> comp_vals;
        std::list<std::string> comp_tmps;
        t_set_Compare.hgetall(&comp_vals, &comp_tmps);
        compareResultList.resize(vals.size());
        itV = set_intersection(vals.begin(), vals.end(), comp_vals.begin(), \
            comp_vals.end(), compareResultList.begin());
        std::list<std::string>::iterator itTmp = compareResultList.begin();
        int sizeTmp = 0;
        while (itTmp != itV) {
            ++sizeTmp;
            ++itTmp;
        }
        compareResultList.resize(sizeTmp);
        if (compareResultList.empty()) {
            packet->sendBuff.append(":0\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        vals = compareResultList;
    }

    std::list<std::string>::const_iterator itList = compareResultList.begin();
    for (; itList != compareResultList.end(); ++itList) {
        t_set_first.hset(*itList, *itList);
    }
    packet->sendBuff.appendFormatString(":%d\r\n", compareResultList.size());
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSMembersCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string hashName(r.tokens[1].s, r.tokens[1].len);
    TSet set(packet->proxy()->leveldbCluster(), hashName);

    KeyValues result;
    set.hgetall(&result);
    packet->sendBuff.appendFormatString("*%d\r\n", result.size());
    KeyValues::iterator it = result.begin();
    for (; it != result.end(); ++it) {
        KeyValue& item = *it;
        packet->sendBuff.appendFormatString("$%d\r\n", item.first.size());
        packet->sendBuff.append(item.first.data(), item.first.size());
        packet->sendBuff.append("\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSMoveCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string src(r.tokens[1].s, r.tokens[1].len);
    std::string dest(r.tokens[2].s, r.tokens[2].len);
    std::string member(r.tokens[3].s, r.tokens[3].len);

    TSet src_set(packet->proxy()->leveldbCluster(), src);
    TSet src_dest(packet->proxy()->leveldbCluster(), dest);

    std::string value;
    if (!src_set.hget(member, &value)) {
        packet->sendBuff.append(":0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    if (!src_set.hdel(member)) {
        packet->sendBuff.append(":0\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    src_dest.hset(member, member);
    packet->sendBuff.append(":1\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSPopCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string name(r.tokens[1].s, r.tokens[1].len);
    TSet set(packet->proxy()->leveldbCluster(), name);

    KeyValues result;
    set.hgetall(&result);

    if (result.empty()) {
        packet->sendBuff.append("$-1\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int index = (rand() % result.size()) - 1;
    KeyValues::iterator it = result.begin();
    for (int i = 0; i <= index; ++it, ++i) {}

    std::string& member = (*it).first;
    set.hdel(member);

    packet->sendBuff.appendFormatString("$%d\r\n", member.size());
    packet->sendBuff.append(member.data(), member.size());
    packet->sendBuff.append("\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSRandMember(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string name(r.tokens[1].s, r.tokens[1].len);
    TSet set(packet->proxy()->leveldbCluster(), name);

    KeyValues result;
    set.hgetall(&result);

    if (result.empty()) {
        packet->sendBuff.append("$-1\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    int index = (rand() % result.size()) - 1;
    KeyValues::iterator it = result.begin();
    for (int i = 0; i <= index; ++it, ++i) {}

    std::string& member = (*it).first;

    packet->sendBuff.appendFormatString("$%d\r\n", member.size());
    packet->sendBuff.append(member.data(), member.size());
    packet->sendBuff.append("\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSRemCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    std::string name(r.tokens[1].s, r.tokens[1].len);
    TSet set(packet->proxy()->leveldbCluster(), name);
    int succeed = 0;

    for (int i = 2; i < r.tokenCount; ++i) {
        std::string member(r.tokens[i].s, r.tokens[i].len);
        if (set.hexists(member)) {
            set.hdel(member);
            ++succeed;
        }
    }
    packet->sendBuff.appendFormatString(":%d\r\n", succeed);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSUnionCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::set<std::string> tmp;
    stringlist result;
    for (int i = 1; i < r.tokenCount; ++i) {
        std::string name(r.tokens[i].s, r.tokens[i].len);
        TSet set(packet->proxy()->leveldbCluster(), name);

        KeyValues kvs;
        set.hgetall(&kvs);

        KeyValues::iterator it = kvs.begin();
        for (; it != kvs.end(); ++it) {
            KeyValue& item = *it;
            if (tmp.find(item.first) == tmp.cend()) {
                result.push_back(item.first);
                tmp.insert(item.first);
            }
        }
    }

    packet->sendBuff.appendFormatString("*%d\r\n", result.size());
    for (stringlist::iterator it = result.begin(); it != result.end(); ++it) {
        std::string& s = (*it);
        packet->sendBuff.appendFormatString("$%d\r\n", s.length());
        packet->sendBuff.append(s.data(), s.size());
        packet->sendBuff.append("\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onSUnionStoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::set<std::string> tmp;
    stringlist result;
    for (int i = 2; i < r.tokenCount; ++i) {
        std::string name(r.tokens[i].s, r.tokens[i].len);
        TSet set(packet->proxy()->leveldbCluster(), name);

        KeyValues kvs;
        set.hgetall(&kvs);

        KeyValues::iterator it = kvs.begin();
        for (; it != kvs.end(); ++it) {
            KeyValue& item = *it;
            if (tmp.find(item.first) == tmp.cend()) {
                result.push_back(item.first);
                tmp.insert(item.first);
            }
        }
    }

    std::string name(r.tokens[1].s, r.tokens[1].len);
    TSet store(packet->proxy()->leveldbCluster(), name);
    store.hclear();

    for (stringlist::iterator it = result.begin(); it != result.end(); ++it) {
        std::string& s = (*it);
        store.hset(s, s);
    }

    packet->sendBuff.appendFormatString(":%d\r\n", result.size());
    packet->setFinishedState(ClientPacket::RequestFinished);
}

void onSClearCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    std::string hashName(r.tokens[1].s, r.tokens[1].len);
    TSet set(packet->proxy()->leveldbCluster(), hashName);
    set.hclear();

    packet->sendBuff.append("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}



//ZSET
void onZAddCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else if (r.tokenCount % 2 != 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        for (int i = 2; i < r.tokenCount; i += 2) {
            if (!TRedisHelper::isDouble(r.tokens[i].s, r.tokens[i].len)) {
                packet->sendBuff.append("-ERR value is not a valid float\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
                return;
            }
        }

        int succeed = 0;
        for (int i = 2; i < r.tokenCount; i += 2) {
            std::string score(r.tokens[i].s, r.tokens[i].len);
            std::string element(r.tokens[i+1].s, r.tokens[i+1].len);
            if (zset.hexists(element)) {
                zset.zadd(atof(score.c_str()), element);
            } else {
                if (zset.zadd(atof(score.c_str()), element)) {
                    ++succeed;
                }
            }
        }
        packet->sendBuff.appendFormatString(":%d\r\n", succeed);
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRemCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);

        int succeed = 0;
        for (int i = 2; i < r.tokenCount; ++i) {
            std::string element(r.tokens[i].s, r.tokens[i].len);
            if (zset.zrem(element)) {
                ++succeed;
            }
        }
        packet->sendBuff.appendFormatString(":%d\r\n", succeed);
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZIncrbyCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_score(r.tokens[2].s, r.tokens[2].len);
        std::string element(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isDouble(str_score)) {
            packet->sendBuff.append("-ERR value is not a valid float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        double score = atof(str_score.c_str());
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        zset.zincrby(element, score);

        char buf[32];
        TRedisHelper::doubleToString(buf, score);

        packet->sendBuff.appendFormatString("$%d\r\n%s\r\n", strlen(buf), buf);
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRankCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string element(r.tokens[2].s, r.tokens[2].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        int index = zset.zrank(element);
        if (index < 0) {
            packet->sendBuff.appendFormatString("$-1\r\n");
        } else {
            packet->sendBuff.appendFormatString(":%d\r\n", index);
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRevRankCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string element(r.tokens[2].s, r.tokens[3].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        int index = zset.zrevrank(element);
        if (index < 0) {
            packet->sendBuff.appendFormatString("$-1\r\n");
        } else {
            packet->sendBuff.appendFormatString(":%d\r\n", index);
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRangeCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        bool withscores = false;
        for (int i = 4; i < r.tokenCount; ++i) {
            if (strncasecmp(r.tokens[i].s, "withscores", 10) == 0) {
                withscores = true;
            } else {
                packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                return;
            }
        }

        std::list<ZSetItem> result;
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_start(r.tokens[2].s, r.tokens[2].len);
        std::string str_stop(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isInteger(str_start) || !TRedisHelper::isInteger(str_stop)) {
            packet->sendBuff.append("-value is not an integer or out of range\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        zset.zrange(atoi(str_start.c_str()), atoi(str_stop.c_str()), &result);
        packet->sendBuff.appendFormatString("*%d\r\n", withscores ? result.size() * 2 : result.size());
        for (std::list<ZSetItem>::iterator it = result.begin(); it != result.end(); ++it) {
            ZSetItem& item = *it;
            packet->sendBuff.appendFormatString("$%d\r\n", item.name.length());
            packet->sendBuff.append(item.name.data(), item.name.length());
            packet->sendBuff.append("\r\n");

            if (withscores) {
                char buf[32];
                int len = TRedisHelper::doubleToString(buf, item.score);
                packet->sendBuff.appendFormatString("$%d\r\n", len);
                packet->sendBuff.append(buf, len);
                packet->sendBuff.append("\r\n");
            }
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRevRangeCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        bool withscores = false;
        for (int i = 4; i < r.tokenCount; ++i) {
            if (strncasecmp(r.tokens[i].s, "withscores", 10) == 0) {
                withscores = true;
            } else {
                packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                return;
            }
        }

        std::list<ZSetItem> result;
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_start(r.tokens[2].s, r.tokens[2].len);
        std::string str_stop(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isInteger(str_start) || !TRedisHelper::isInteger(str_stop)) {
            packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        zset.zrevrange(atoi(str_start.c_str()), atoi(str_stop.c_str()), &result);
        packet->sendBuff.appendFormatString("*%d\r\n", withscores ? result.size() * 2 : result.size());
        for (std::list<ZSetItem>::iterator it = result.begin(); it != result.end(); ++it) {
            ZSetItem& item = *it;
            packet->sendBuff.appendFormatString("$%d\r\n", item.name.length());
            packet->sendBuff.append(item.name.data(), item.name.length());
            packet->sendBuff.append("\r\n");

            if (withscores) {
                char buf[32];
                int len = TRedisHelper::doubleToString(buf, item.score);
                packet->sendBuff.appendFormatString("$%d\r\n", len);
                packet->sendBuff.append(buf, len);
                packet->sendBuff.append("\r\n");
            }
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRangeByScoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        bool withscores = false;
        bool limit = false;
        int limitoffset = 0;
        int limitcount = 0;
        for (int i = 4; i < r.tokenCount; ) {
            char* opt = r.tokens[i].s;
            if (strncasecmp(opt, "withscores", 10) == 0) {
                withscores = true;
                ++i;
            } else if (strncasecmp(opt, "limit", 5) == 0) {
                if (r.tokenCount - i < 3) {
                    packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                    return;
                }
                limit = true;
                std::string str_offset(r.tokens[i+1].s, r.tokens[i+1].len);
                std::string str_count(r.tokens[i+2].s, r.tokens[i+2].len);

                if (!TRedisHelper::isInteger(str_offset) || !TRedisHelper::isInteger(str_count)) {
                    packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
                    packet->setFinishedState(ClientPacket::RequestFinished);
                    return;
                }

                limitoffset = atoi(str_offset.c_str());
                limitcount = atoi(str_count.c_str());
                i += 3;
            } else {
                packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                return;
            }
        }

        std::list<ZSetItem> result;
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_min_score(r.tokens[2].s, r.tokens[2].len);
        std::string str_max_score(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isDouble(str_min_score) || !TRedisHelper::isDouble(str_min_score)) {
            packet->sendBuff.append("-ERR min or max is not a float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        double min_score = atof(str_min_score.c_str());
        double max_score = atof(str_max_score.c_str());

        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        zset.zrangebyscore(min_score, max_score, &result);

        if (limit) {
            if (limitoffset < 0 || limitoffset >= (int)result.size()) {
                packet->sendBuff.appendFormatString("*0\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
                return;
            }
            if (limitcount < 0 || limitcount > (int)result.size()) {
                limitcount = (int)result.size() - limitoffset;
            }
        } else {
            limitoffset = 0;
            limitcount = result.size();
        }

        packet->sendBuff.appendFormatString("*%d\r\n", withscores ? limitcount * 2 : limitcount);
        std::list<ZSetItem>::iterator it = result.begin();
        for (int i = 0; it != result.end(); ++it, ++i) {
            if (i >= limitoffset) {
                if (i - limitoffset >= limitcount) {
                    break;
                }
                ZSetItem& item = *it;
                packet->sendBuff.appendFormatString("$%d\r\n", item.name.length());
                packet->sendBuff.append(item.name.data(), item.name.length());
                packet->sendBuff.append("\r\n");

                if (withscores) {
                    char buf[32];
                    int len = TRedisHelper::doubleToString(buf, item.score);
                    packet->sendBuff.appendFormatString("$%d\r\n", len);
                    packet->sendBuff.append(buf, len);
                    packet->sendBuff.append("\r\n");
                }
            }
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}

void onZRevRangeByScoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount < 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        bool withscores = false;
        bool limit = false;
        int limitoffset = 0;
        int limitcount = 0;
        for (int i = 4; i < r.tokenCount; ) {
            char* opt = r.tokens[i].s;
            if (strncasecmp(opt, "withscores", 10) == 0) {
                withscores = true;
                ++i;
            } else if (strncasecmp(opt, "limit", 5) == 0) {
                if (r.tokenCount - i < 3) {
                    packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                    return;
                }
                limit = true;
                std::string str_offset(r.tokens[i+1].s, r.tokens[i+1].len);
                std::string str_count(r.tokens[i+2].s, r.tokens[i+2].len);

                if (!TRedisHelper::isInteger(str_offset) || !TRedisHelper::isInteger(str_count)) {
                    packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
                    packet->setFinishedState(ClientPacket::RequestFinished);
                    return;
                }

                limitoffset = atoi(str_offset.c_str());
                limitcount = atoi(str_count.c_str());
                i += 3;
            } else {
                packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
                return;
            }
        }

        std::list<ZSetItem> result;
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_min_score(r.tokens[3].s, r.tokens[3].len);
        std::string str_max_score(r.tokens[2].s, r.tokens[2].len);

        if (!TRedisHelper::isDouble(str_min_score) || !TRedisHelper::isDouble(str_min_score)) {
            packet->sendBuff.append("-ERR min or max is not a float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        double min_score = atof(str_min_score.c_str());
        double max_score = atof(str_max_score.c_str());

        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        zset.zrevrangebyscore(max_score, min_score, &result);

        if (limit) {
            if (limitoffset < 0 || limitoffset >= (int)result.size()) {
                packet->sendBuff.appendFormatString("*0\r\n");
                packet->setFinishedState(ClientPacket::RequestFinished);
                return;
            }
            if (limitcount < 0 || limitcount > (int)result.size()) {
                limitcount = (int)result.size() - limitoffset;
            }
        } else {
            limitoffset = 0;
            limitcount = result.size();
        }

        packet->sendBuff.appendFormatString("*%d\r\n", withscores ? limitcount * 2 : limitcount);
        std::list<ZSetItem>::iterator it = result.begin();
        for (int i = 0; it != result.end(); ++it, ++i) {
            if (i >= limitoffset) {
                if (i - limitoffset >= limitcount) {
                    break;
                }
                ZSetItem& item = *it;
                packet->sendBuff.appendFormatString("$%d\r\n", item.name.length());
                packet->sendBuff.append(item.name.data(), item.name.length());
                packet->sendBuff.append("\r\n");

                if (withscores) {
                    char buf[32];
                    int len = TRedisHelper::doubleToString(buf, item.score);
                    packet->sendBuff.appendFormatString("$%d\r\n", len);
                    packet->sendBuff.append(buf, len);
                    packet->sendBuff.append("\r\n");
                }
            }
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}

void onZCountCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        std::string str_min_score(r.tokens[2].s, r.tokens[2].len);
        std::string str_max_score(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isDouble(str_min_score) || !TRedisHelper::isDouble(str_max_score)) {
            packet->sendBuff.append("-ERR min or max is not a float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        double min_score = atof(str_min_score.c_str());
        double max_score = atof(str_max_score.c_str());

        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        packet->sendBuff.appendFormatString(":%d\r\n", zset.zcount(min_score, max_score));
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZCardCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        packet->sendBuff.appendFormatString(":%d\r\n", zset.zcard());
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZSCoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        std::string element(r.tokens[2].s, r.tokens[2].len);
        double score = 0;
        if (!zset.zscore(element, &score)) {
            packet->sendBuff.appendFormatString("$-1\r\n");
        } else {
            char buf[32];
            int len = TRedisHelper::doubleToString(buf, score);
            packet->sendBuff.appendFormatString("$%d\r\n%s\r\n", len, buf);
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRemRangeByRankCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        std::string str_start(r.tokens[2].s, r.tokens[2].len);
        std::string str_stop(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isInteger(str_start) || !TRedisHelper::isInteger(str_stop)) {
            packet->sendBuff.append("-ERR value is not an integer or out of range\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        int start = atoi(str_start.c_str());
        int stop = atoi(str_stop.c_str());
        int ret = zset.zremrangebyrank(start, stop);
        packet->sendBuff.appendFormatString(":%d\r\n", ret);
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}


void onZRemRangeByScoreCommand(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        std::string str_minscore(r.tokens[2].s, r.tokens[2].len);
        std::string str_maxscore(r.tokens[3].s, r.tokens[3].len);

        if (!TRedisHelper::isDouble(str_minscore) || !TRedisHelper::isDouble(str_maxscore)) {
            packet->sendBuff.append("-ERR min or max is not a float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }

        double minscore = atof(str_minscore.c_str());
        double maxscore = atof(str_maxscore.c_str());
        int ret = zset.zremrangebyscore(minscore, maxscore);
        packet->sendBuff.appendFormatString(":%d\r\n", ret);
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}

void onZClear(ClientPacket *packet, void *)
{
    RedisProtoParseResult& r = packet->recvParseResult;
    if (r.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
    } else {
        std::string setname(r.tokens[1].s, r.tokens[1].len);
        TZSet zset(packet->proxy()->leveldbCluster(), setname);
        if (zset.zremrangebyrank(0, -1) > 0) {
            packet->sendBuff.appendFormatString("+OK\r\n");
        } else {
            packet->sendBuff.appendFormatString("-ERR clear failed\r\n");
        }
        packet->setFinishedState(ClientPacket::RequestFinished);
    }
}
