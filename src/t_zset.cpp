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

#include "t_zset.h"

bool operator < (const ZSetItem& lhs, const ZSetItem& rhs)
{
    return lhs.score < rhs.score;
}

TZSet::TZSet(LeveldbCluster* db, const std::string &name) :
    THash(db, name)
{
    m_internalType = T_ZSet;
}

TZSet::~TZSet(void)
{
}

bool TZSet::zadd(double score, const std::string &element)
{
    std::string value;
    value.assign((char*)&score, sizeof(score));
    return hset(element, value);
}

bool TZSet::zrem(const std::string &element)
{
    return hdel(element);
}

bool TZSet::zincrby(const std::string &element, double& num)
{
    std::string val;
    if (!hget(element, &val)) {
        if (!zadd(num, element)) {
            return false;
        }
    }

    double oldScore = *((double*)val.data());
    double newScore = oldScore + num;
    num = newScore;
    return zadd(newScore, element);
}

int TZSet::zrank(const std::string &element)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    int index = 0;
    ZSetItemList::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        ZSetItem& item = *it;
        if (item.name == element) {
            return index;
        }
        ++index;
    }
    return -1;
}

int TZSet::zrevrank(const std::string &element)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    int index = 0;
    ZSetItemList::reverse_iterator it = items.rbegin();
    for (; it != items.rend(); ++it) {
        ZSetItem& item = *it;
        if (item.name == element) {
            return index;
        }
        ++index;
    }
    return -1;
}

bool TZSet::zrange(int start, int stop, ZSetItemList* result)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    std::vector<ZSetItem> src(items.size());
    ZSetItemList::iterator it = items.begin();
    for (int i = 0; it != items.end(); ++it, ++i) {
        src[i] = *it;
    }

    if (!transformIndex(start, stop, src.size())) {
        return false;
    }
    for (int i = start; i <= stop; ++i) {
        result->push_back(src[i]);
    }
    return true;
}

bool TZSet::zrevrange(int start, int stop, ZSetItemList* result)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    std::vector<ZSetItem> src(items.size());
    ZSetItemList::reverse_iterator it = items.rbegin();
    for (int i = 0; it != items.rend(); ++it, ++i) {
        src[i] = *it;
    }

    if (!transformIndex(start, stop, src.size())) {
        return false;
    }
    for (int i = start; i <= stop; ++i) {
        result->push_back(src[i]);
    }
    return true;
}

bool TZSet::zrangebyscore(double min_score, double max_score, ZSetItemList* result)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    ZSetItemList::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        ZSetItem& item = *it;
        if (item.score >= min_score && item.score <= max_score) {
            result->push_back(item);
        }
    }
    return true;
}

bool TZSet::zrevrangebyscore(double max_score, double min_score, std::list<ZSetItem> *result)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    ZSetItemList::reverse_iterator it = items.rbegin();
    for (; it != items.rend(); ++it) {
        ZSetItem& item = *it;
        if (item.score >= min_score && item.score <= max_score) {
            result->push_back(item);
        }
    }
    return true;
}

int TZSet::zcount(double min_score, double max_score)
{
    ZSetItemList items;
    query(&items);

    int count = 0;
    ZSetItemList::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        ZSetItem& item = *it;
        if (item.score >= min_score && item.score <= max_score) {
            ++count;
        }
    }
    return count;
}

int TZSet::zcard(void)
{
    ZSetItemList items;
    query(&items);
    return items.size();
}

bool TZSet::zscore(const std::string &element, double *score)
{
    ZSetItemList items;
    query(&items);

    ZSetItemList::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        ZSetItem& item = *it;
        if (item.name == element) {
            *score = item.score;
            return true;
        }
    }
    return false;
}

int TZSet::zremrangebyrank(int start, int stop)
{
    ZSetItemList items;
    query(&items);

    items.sort();
    std::vector<ZSetItem> src(items.size());
    ZSetItemList::iterator it = items.begin();
    for (int i = 0; it != items.end(); ++it, ++i) {
        src[i] = *it;
    }

    int result = 0;
    if (!transformIndex(start, stop, src.size())) {
        return result;
    }

    for (int i = start; i <= stop; ++i) {
        ZSetItem& item = src[i];
        if (hdel(item.name)) {
            ++result;
        }
    }
    return result;
}

int TZSet::zremrangebyscore(double min_score, double max_score)
{
    ZSetItemList items;
    query(&items);

    int result = 0;
    ZSetItemList::iterator it = items.begin();
    for (; it != items.end(); ++it) {
        ZSetItem& item = *it;
        if (item.score >= min_score && item.score <= max_score) {
            if (hdel(item.name)) {
                ++result;
            }
        }
    }
    return result;
}

void TZSet::query(ZSetItemList *result)
{
    KeyValues lst;
    hgetall(&lst);
    KeyValues::iterator it = lst.begin();
    for (; it != lst.end(); ++it) {
        KeyValue& item = *it;
        ZSetItem tmp;
        tmp.name = item.first;
        tmp.score = *((double*)item.second.data());
        result->push_back(tmp);
    }
}

bool TZSet::transformIndex(int& start, int& stop, int maxsize)
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


