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

#include "t_list.h"

bool tlist_transformIndex(int& start, int& stop, int maxsize)
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

TList::TList(LeveldbCluster *db, const std::string &name) :
    m_db(db),
    m_listname(name)
{
}

TList::~TList(void)
{
}


bool TList::lindex(int index, std::string* value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string valueTmp;
    m_db->value(key, valueTmp);
    if (valueTmp.empty()) {
        setLastError("no such key");
        return false;
    }
    ListValueBuffer* listBuff = (ListValueBuffer*) valueTmp.data();

    int size = listBuff->size;
    if(index < 0)
        index += size;
    index += listBuff->left_pos + 1;

    if (index < (listBuff->left_pos + 1) || index > (listBuff->right_pos - 1)) {
        setLastError("index out of range");
        return false;
    }

    std::string keyTmp;
    ListElementKey::makeListElementKey(m_listname, index, keyTmp);
    m_db->value(XObject(keyTmp.data(), keyTmp.size()), *value);
    return value;
}

int TList::llen(void)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string valueTmp;
    m_db->value(key, valueTmp);
    if (valueTmp.empty()) {
        setLastError("no such key");
        return 0;
    }
    ListValueBuffer* listBuff = (ListValueBuffer*) valueTmp.data();
    return (listBuff->size < 0 ? 0 : listBuff->size);
}

// pop and return the list's first key
bool TList::lpop(std::string* value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string valueTmp;
    m_db->value(key, valueTmp);
    if (valueTmp.empty()) {
        setLastError("no such key");
        return false;
    }

    const ListValueBuffer* listBuff = (ListValueBuffer*) valueTmp.data();
    if (listBuff->size <= 0) {
        return false;
    }

    std::string keyTmp;
    ListElementKey::makeListElementKey(m_listname, listBuff->left_pos + 1, keyTmp);

    // get the value
    m_db->value(XObject(keyTmp.data(), keyTmp.size()), *value);
    // del the value
    m_db->remove(XObject(keyTmp.data(), keyTmp.size()));

    // update the valueList
    ListValueBuffer update;
    update.counter = listBuff->counter - 1;
    update.size = listBuff->size - 1;
    update.left_pos = listBuff->left_pos + 1;
    update.right_pos = listBuff->right_pos;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));

    return value;
}


int TList::lpush(const std::string &value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string oldValue;
    if (!m_db->value(key, oldValue)) {
        ListValueBuffer listBuf;
        listBuf.counter = 0;
        listBuf.size = 0;
        listBuf.left_pos = 0;
        listBuf.right_pos = 1;
        oldValue.assign((char*)&listBuf, sizeof(ListValueBuffer));
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->left_pos;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject data_key(selement.data(), selement.size());
    XObject data_val(value.data(), value.size());
    m_db->setValue(data_key, data_val);

    ListValueBuffer update;
    update.counter = valueBuff->counter + 1;
    update.size = valueBuff->size + 1;
    update.left_pos = valueBuff->left_pos - 1;
    update.right_pos = valueBuff->right_pos;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));
    return update.size;
}


int TList::lpushx(const std::string &value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string oldValue;
    if (!m_db->value(key, oldValue)) {
        setLastError("no such key");
        return 0;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->left_pos;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject data_key(selement.data(), selement.size());
    XObject data_val(value.data(), value.size());
    m_db->setValue(data_key, data_val);

    ListValueBuffer update;
    update.counter = valueBuff->counter + 1;
    update.size = valueBuff->size + 1;
    update.left_pos = valueBuff->left_pos - 1;
    update.right_pos = valueBuff->right_pos;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));
    return update.size;
}


bool TList::lrange(int start, int stop, stringlist *result)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string valueTmp;
    if (!m_db->value(key, valueTmp)) {
        return false;
    }

    ListValueBuffer* listBuff = (ListValueBuffer*) valueTmp.data();
    int size = listBuff->size;
    if (!tlist_transformIndex(start, stop, size)) {
        return false;
    }

    for(int i= start; i <= stop; ++i) {
        int index = i;
        
        if (index < 0)
            index += listBuff->size;
        index += listBuff->left_pos + 1;
                
        if (index < (listBuff->left_pos + 1) || index > (listBuff->right_pos - 1)) {
            setLastError("index out of range");
            return false;
        }
        std::string keyTmp;
        ListElementKey::makeListElementKey(m_listname, index, keyTmp);
        std::string val_;
        XObject xobKey(keyTmp.data(), keyTmp.size());
        m_db->value(xobKey, val_);
        result->push_back(val_);
    }
    return true;
}

bool TList::lset(int index, const std::string &value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string valueTmp;
    if (!m_db->value(key, valueTmp)) {
        setLastError("no such key");
        return false;
    }
    ListValueBuffer* valueBuff = (ListValueBuffer*) valueTmp.data();

    if (index < 0)
        index += valueBuff->size;
    index += valueBuff->left_pos + 1;

    if (index < (valueBuff->left_pos + 1) || index > (valueBuff->right_pos - 1)) {
        setLastError("index out of range");
        return false;
    }
    
    std::string selement;
    ListElementKey::makeListElementKey(m_listname, index, selement);
    m_db->setValue(XObject(selement.data(), selement.size()), XObject(value.data(), value.size()));
    return true;
}

bool TList::ltrim(int start, int stop)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string valueTmp;
    if (!m_db->value(key, valueTmp)) {
        setLastError("no such key");
        return false;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*) valueTmp.data();
    if (!tlist_transformIndex(start, stop, valueBuff->size)) {
        setLastError("index out of range");
        return false;
    }

    if (start < 0)
        start += valueBuff->size;
    start += valueBuff->left_pos + 1;

    if (start < (valueBuff->left_pos + 1) || start > (valueBuff->right_pos - 1)) {
        setLastError("index out of range");
        return false;
    }

    if (stop < 0)
        stop += valueBuff->size;
    stop += valueBuff->left_pos + 1;

    if (stop < (valueBuff->left_pos + 1) || stop > (valueBuff->right_pos - 1)) {
        setLastError("index out of range");
        return false;
    }
    if (start > stop) {
        start = valueBuff->size;
    }

    int removeCount = 0;
    for (int i = valueBuff->left_pos+1; i < valueBuff->right_pos; ++i) {
        if (i < start || i > stop) {
            std::string selement;
            ListElementKey::makeListElementKey(m_listname, i, selement);
            XObject elementKey(selement.data(), selement.size());
            m_db->remove(elementKey);
            ++removeCount;
        }
    }

    // update the valueList
    ListValueBuffer update;
    update.counter = valueBuff->counter - removeCount;
    update.size = valueBuff->size - removeCount;
    update.left_pos = start;
    update.right_pos = stop;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));
    return true;
}

bool TList::rpop(std::string* value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string valueTmp;
    m_db->value(key, valueTmp);
    if (valueTmp.empty()) {
        setLastError("no such key");
        return false;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*) valueTmp.data();
    if (valueBuff->size <= 0) {
        return false;
    }

    std::string keyTmp;
    ListElementKey::makeListElementKey(m_listname, valueBuff->right_pos - 1, keyTmp);

    // get the value
    m_db->value(XObject(keyTmp.data(), keyTmp.size()), *value);
    // del the value
    m_db->remove(XObject(keyTmp.data(), keyTmp.size()));

    // update the valueList
    ListValueBuffer update;
    update.counter = valueBuff->counter - 1;
    update.size = valueBuff->size - 1;
    update.left_pos = valueBuff->left_pos;
    update.right_pos = valueBuff->right_pos - 1;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));

    return value;
}

int TList::rpush(const std::string &value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());

    std::string oldValue;
    if (!m_db->value(key, oldValue)) {
        ListValueBuffer init;
        init.counter = 0;
        init.size = 0;
        init.left_pos = 0;
        init.right_pos = 1;
        oldValue.assign((char*)&init, sizeof(ListValueBuffer));
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->right_pos;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject data_key(selement.data(), selement.size());
    XObject data_val(value.data(), value.size());
    m_db->setValue(data_key, data_val);

    ListValueBuffer update;
    update.counter = valueBuff->counter + 1;
    update.size = valueBuff->size + 1;
    update.left_pos = valueBuff->left_pos;
    update.right_pos = valueBuff->right_pos + 1;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));
    return update.size;
}

int TList::rpushx(const std::string &value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string oldValue;
    if (!m_db->value(key, oldValue)) {
        setLastError("no such key");
        return 0;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->right_pos;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject data_key(selement.data(), selement.size());
    XObject data_val(value.data(), value.size());
    m_db->setValue(data_key, data_val);

    ListValueBuffer update;
    update.counter = valueBuff->counter + 1;
    update.size = valueBuff->size + 1;
    update.left_pos = valueBuff->left_pos;
    update.right_pos = valueBuff->right_pos + 1;

    std::string newValue;
    newValue.assign((char *)&update, sizeof(ListValueBuffer));
    m_db->setValue(key, XObject(newValue.data(), newValue.size()));
    return update.size;
}

void TList::lclear(void)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        return;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)val.data();
    for (int i = 0; i < valueBuff->size; ++i) {
        std::string selement;
        ListElementKey::makeListElementKey(m_listname, valueBuff->at(i), selement);
        XObject elementKey(selement.data(), selement.size());
        m_db->remove(elementKey);
    }
    m_db->remove(listkey);
}