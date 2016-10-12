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


bool TList::lindex(int index, std::string *value)
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
    if (index < 0) {
        index += size;
    }
    if (index < 0 || index > size -  1) {
        setLastError("index out of range");
        return false;
    }
    std::string eleKey;
    ListElementKey::makeListElementKey(m_listname, listBuff->at(index), eleKey);
    m_db->value(XObject(eleKey.data(), eleKey.size()), *value);
    return true;
}


bool TList::linsert(int pos, const std::string &value)
{
    std::string _listKey;
    ListKey::makeListKey(m_listname, _listKey);
    XObject listkey(_listKey.data(), _listKey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        setLastError("no such key");
        return false;
    }
    const ListValueBuffer* listValueBuff = (ListValueBuffer*) val.data();
    int newCnt = listValueBuff->counter + 1;
    const int size = listValueBuff->size;
    if (pos < 0 || pos > size || size <= 0) {
        setLastError("pos error");
        return false;
    }
    IOBuffer b;
    ListValueBuffer newListV;
    newListV.size = size + 1;
    newListV.counter = newCnt;
    b.append((char*)&newListV, sizeof(ListValueBuffer));
    b.append((char*)listValueBuff->intBuffer(), pos * sizeof(int));
    b.append((char*)&newListV.counter, sizeof(int));
    b.append((char*)(listValueBuff->intBuffer() + pos), (size - pos) * sizeof(int));
    if (!m_db->setValue(listkey, XObject(b.data(), b.size()))) {
        setLastError("set value error");
        return false;
    }
    // insert the ele_key
    std::string eleKey;
    ListElementKey::makeListElementKey(m_listname, newCnt, eleKey);
    m_db->setValue(XObject(eleKey.data(), eleKey.size()), XObject(value.data(), value.size()));
    return true;
}


int TList::llen(void)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string value;
    m_db->value(key, value);
    if (value.empty()) {
        setLastError("no such key");
        return 0;
    }
    ListValueBuffer* listBuff = (ListValueBuffer*)value.data();
    return (listBuff->size < 0 ? 0 : listBuff->size);
}

// pop the list's first key
bool TList::lpop(std::string *value)
{
    std::string buf;
    ListKey::makeListKey(m_listname, buf);
    XObject key(buf.data(), buf.size());
    std::string listVal;
    m_db->value(key, listVal);
    if (listVal.empty()) {
        setLastError("no such key");
        return false;
    }

    const ListValueBuffer* listBuff = (ListValueBuffer*) listVal.data();
    if (listBuff->size <= 0) {
        return false;
    }
    // get the first num
    std::string keyTmp;
    ListElementKey::makeListElementKey(m_listname, listBuff->at(0), keyTmp);
    m_db->value(XObject(keyTmp.data(), keyTmp.size()), *value);
    // del the key
    m_db->remove(XObject(keyTmp.data(), keyTmp.size()));

    // change the valueList
    ListValueBuffer listVBuff;
    listVBuff.counter = listBuff->counter;
    listVBuff.size = listBuff->size - 1;
    IOBuffer bufNew;
    bufNew.append((char*)&listVBuff, sizeof(ListValueBuffer));
    bufNew.append((char*)(listBuff->intBuffer() + 1), listVBuff.size * sizeof(int));
    // set new valueList;
    m_db->setValue(key, XObject(bufNew.data(), bufNew.size()));
    return true;
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
        oldValue.assign((char*)&listBuf, sizeof(ListValueBuffer));
    }
    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    int* oldIntList = valueBuff->intBuffer();
    ListValueBuffer newVBuf;
    newVBuf.counter = valueBuff->counter + 1;
    newVBuf.size = valueBuff->size + 1;
    IOBuffer bufNew;
    bufNew.append((char*)&newVBuf, sizeof(ListValueBuffer));
    bufNew.append((char*)&newVBuf.counter, sizeof(int));
    bufNew.append((char*)oldIntList, sizeof(int) * valueBuff->size);
    m_db->setValue(key, XObject(bufNew.data(), bufNew.size()));

    // set ele_key
    std::string realKey;
    ListElementKey::makeListElementKey(m_listname, valueBuff->counter + 1, realKey);
    m_db->setValue(XObject(realKey.data(), realKey.size()), XObject(value.data(), value.size()));
    return valueBuff->size + 1;
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
    int* oldIntList = valueBuff->intBuffer();
    ListValueBuffer newVBuf;
    newVBuf.counter = valueBuff->counter + 1;
    newVBuf.size = valueBuff->size + 1;
    IOBuffer bufNew;
    bufNew.append((char*)&newVBuf, sizeof(ListValueBuffer));
    bufNew.append((char*)&newVBuf.counter, sizeof(int));
    bufNew.append((char*)oldIntList, sizeof(int) * valueBuff->size);
    m_db->setValue(key, XObject(bufNew.data(), bufNew.size()));
    // set ele_key
    std::string realKey;
    ListElementKey::makeListElementKey(m_listname, valueBuff->counter + 1, realKey);
    m_db->setValue(XObject(realKey.data(), realKey.size()), XObject(value.data(), value.size()));

    return valueBuff->size + 1;
}


bool TList::lrange(int start, int stop, stringlist *result)
{
    std::string _listKey;
    ListKey::makeListKey(m_listname, _listKey);
    XObject listkey(_listKey.data(), _listKey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        return false;
    }

    const ListValueBuffer* listValueBuff = (ListValueBuffer*) val.data();
    const int size = listValueBuff->size;
    if (!tlist_transformIndex(start, stop, size)) {
        return false;
    }

    for(int i = start; i <= stop; ++i) {
        int index = listValueBuff->at(i);
        std::string keyTmp;
        ListElementKey::makeListElementKey(m_listname, index, keyTmp);
        std::string val_;
        XObject xobKey(keyTmp.data(), keyTmp.size());
        m_db->value(xobKey, val_);
        result->push_back(val_);
    }
    return true;
}


int TList::lrem(int count, const std::string &value)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        return 0;
    }

    IOBuffer newBuff;
    ListValueBuffer* valueBuff = (ListValueBuffer*)val.data();
    newBuff.append((char*)valueBuff, sizeof(ListValueBuffer));

    int start = 0;
    int step = 1;
    int stop = valueBuff->size - 1;

    if (count < 0) {
        start = valueBuff->size - 1;
        stop = 0;
        step = -1;
        count = -count;
    }
    if (count == 0) {
        count = valueBuff->size;
    }

    int* intBuff = valueBuff->intBuffer();
    int removeCount = 0;
    for (int i = start; ; i += step) {
        int id = valueBuff->at(i);
        if (removeCount < count) {
            std::string selement;
            ListElementKey::makeListElementKey(m_listname, id, selement);
            std::string tmpval;
            XObject elementKey(selement.data(), selement.size());
            m_db->value(elementKey, tmpval);
            if (tmpval == value) {
                m_db->remove(elementKey);
                ++removeCount;
                intBuff[i] = -1;
            }
        }
        if (i == stop) {
            break;
        }
    }
    for (int i = 0; i < valueBuff->size; ++i) {
        int id = valueBuff->at(i);
        if (id != -1) {
            newBuff.append((char*)&id, sizeof(int));
        }
    }
    valueBuff = (ListValueBuffer*)newBuff.data();
    valueBuff->counter += removeCount;
    valueBuff->size -= removeCount;

    m_db->setValue(listkey, XObject(newBuff.data(), newBuff.size()));
    return removeCount;
}

bool TList::lset(int index, const std::string &value)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        setLastError("no such key");
        return false;
    }
    ListValueBuffer* valueBuff = (ListValueBuffer*)val.data();
    if (index < 0) {
        index = valueBuff->size + index;
    }
    if (index < 0 || index > valueBuff->size - 1) {
        setLastError("index out of range");
        return false;
    }
    std::string selement;
    ListElementKey::makeListElementKey(m_listname, valueBuff->at(index), selement);
    m_db->setValue(XObject(selement.data(), selement.size()), XObject(value.data(), value.size()));
    return true;
}

bool TList::ltrim(int start, int stop)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        setLastError("no such key");
        return false;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)val.data();
    if (!tlist_transformIndex(start, stop, valueBuff->size)) {
        setLastError("index out of range");
        return false;
    }

    if (start > stop) {
        start = valueBuff->size;
    }

    IOBuffer newBuff;
    newBuff.append((char*)valueBuff, sizeof(ListValueBuffer));
    int* intBuff = valueBuff->intBuffer();

    int removeCount = 0;
    for (int i = 0; i < valueBuff->size; ++i) {
        if (i < start || i > stop) {
            std::string selement;
            ListElementKey::makeListElementKey(m_listname, intBuff[i], selement);
            XObject elementKey(selement.data(), selement.size());
            m_db->remove(elementKey);
            ++removeCount;
            intBuff[i] = -1;
        }
    }

    for (int i = 0; i < valueBuff->size; ++i) {
        int id = valueBuff->at(i);
        if (id != -1) {
            newBuff.append((char*)&id, sizeof(int));
        }
    }

    valueBuff = (ListValueBuffer*)newBuff.data();
    valueBuff->counter += removeCount;
    valueBuff->size -= removeCount;

    m_db->setValue(listkey, XObject(newBuff.data(), newBuff.size()));
    return true;
}

bool TList::rpop(std::string *value)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string val;
    if (!m_db->value(listkey, val)) {
        return false;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)val.data();
    if (valueBuff->size == 0) {
        return false;
    }

    std::string selement;
    int popElementId = valueBuff->at(valueBuff->size - 1);
    ListElementKey::makeListElementKey(m_listname, popElementId, selement);
    XObject key(selement.data(), selement.size());
    m_db->value(key, *value);
    m_db->remove(key);
    valueBuff->size -= 1;
    valueBuff->counter += 1;
    m_db->setValue(listkey, XObject((char*)valueBuff, valueBuff->bufferSize()));
    return true;
}

int TList::rpush(const std::string &value)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string oldValue;
    if (!m_db->value(listkey, oldValue)) {
        ListValueBuffer init;
        init.counter = 0;
        init.size = 0;
        oldValue.assign((char*)&init, sizeof(ListValueBuffer));
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->counter;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject key(selement.data(), selement.size());
    XObject val(value.data(), value.size());
    m_db->setValue(key, val);

    valueBuff->size += 1;
    valueBuff->counter += 1;

    IOBuffer newValue;
    newValue.append((char*)valueBuff, oldValue.size());
    newValue.append((char*)&newId, sizeof(int));
    m_db->setValue(listkey, XObject(newValue.data(), newValue.size()));
    return valueBuff->size;
}

int TList::rpushx(const std::string &value)
{
    std::string slistkey;
    ListKey::makeListKey(m_listname, slistkey);
    XObject listkey(slistkey.data(), slistkey.size());

    std::string oldValue;
    if (!m_db->value(listkey, oldValue)) {
        return 0;
    }

    ListValueBuffer* valueBuff = (ListValueBuffer*)oldValue.data();
    std::string selement;
    int newId = valueBuff->counter;
    ListElementKey::makeListElementKey(m_listname, newId, selement);

    XObject key(selement.data(), selement.size());
    XObject val(value.data(), value.size());
    m_db->setValue(key, val);

    valueBuff->size += 1;
    valueBuff->counter += 1;

    IOBuffer newValue;
    newValue.append((char*)valueBuff, oldValue.size());
    newValue.append((char*)&newId, sizeof(int));
    m_db->setValue(listkey, XObject(newValue.data(), newValue.size()));
    return valueBuff->size;
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
