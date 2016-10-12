#include "t_hash.h"

void THash::makeHashKey(IOBuffer& buf, HashKeyInfo *info)
{
    buf.appendT(info->type);
    buf.appendT(info->name.len);
    buf.append(info->name.data, info->name.len);
    buf.appendT(info->key.len);
    buf.append(info->key.data, info->key.len);
}

void THash::unmakeHashKey(const char *buf, int, HashKeyInfo *info)
{
    info->type = *((short*)buf);
    int namelen = *((int*)(buf + sizeof(short)));
    info->name = XObject(buf + 6, namelen);

    int keylen = *((int*)(info->name.data + namelen));
    info->key = XObject(info->name.data + info->name.len + sizeof(int), keylen);
}



THash::THash(LeveldbCluster* db, const std::string& name)
{
    m_db = db->mapToDatabase(name.data(), name.size());
    m_dbCluster = db;
    m_hashName = name;
    m_internalType = T_Hash;
}

THash::~THash(void)
{
}

bool THash::hset(const std::string& key, const std::string& value)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    info.key = XObject(key.data(), key.size());
    makeHashKey(buf, &info);

    XObject _key(buf.data(), buf.size());
    XObject _value(value.data(), value.size());

    LeveldbCluster::WriteOption wOp;
    wOp.mapping_key = XObject(m_hashName.data(), m_hashName.size());
    return m_dbCluster->setValue(_key, _value, wOp);
}

bool THash::hget(const std::string& field, std::string* value)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    info.key = XObject(field.data(), field.size());
    makeHashKey(buf, &info);

    XObject key(buf.data(), buf.size());
    LeveldbCluster::ReadOption readOp;
    readOp.mapping_key = XObject(m_hashName.data(), m_hashName.size());
    return m_dbCluster->value(key, *value, readOp);
}


bool THash::hdel(const std::string& field)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    info.key = XObject(field.data(), field.size());
    makeHashKey(buf, &info);
    XObject key(buf.data(), buf.size());

    LeveldbCluster::WriteOption wOp;
    wOp.mapping_key = XObject(m_hashName.data(), m_hashName.size());
    return m_dbCluster->remove(key, wOp);
}


bool THash::hexists(const std::string& field)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    info.key = XObject(field.data(), field.size());
    makeHashKey(buf, &info);
    XObject key(buf.data(), buf.size());
    std::string value;
    LeveldbCluster::ReadOption readOp;
    readOp.mapping_key = XObject(m_hashName.data(), m_hashName.size());
    m_dbCluster->value(key, value, readOp);
    bool isNull = value.empty();
    return !isNull;
}

// get field's num
int THash::hlen(void)
{
    KeyValues result;
    hgetall(&result);
    return result.size();
}

void THash::hgetall(KeyValues* result)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    makeHashKey(buf, &info);

    XObject key(buf.data(), buf.size());
    LeveldbIterator it;
    m_db->initIterator(it);
    it.seek(key);
    while (it.isValid())
    {
        XObject key = it.key();
        HashKeyInfo tmp;
        unmakeHashKey(key.data, key.len, &tmp);
        std::string name(tmp.name.data, tmp.name.len);
        std::string keyname(tmp.key.data, tmp.key.len);
        if (tmp.type != m_internalType || name != m_hashName) {
            break;
        }

        KeyValue item;
        item.first = keyname;
        item.second.assign(it.value().data, it.value().len);
        result->push_back(item);

        it.next();
    }
}

void THash::hgetall(stringlist* keys, stringlist* vals)
{
    IOBuffer buf;
    HashKeyInfo info;
    info.type = m_internalType;
    info.name = XObject(m_hashName.data(), m_hashName.size());
    makeHashKey(buf, &info);

    XObject key(buf.data(), buf.size());
    LeveldbIterator it;
    m_db->initIterator(it);
    it.seek(key);
    while (it.isValid())
    {
        XObject key = it.key();
        HashKeyInfo tmp;
        unmakeHashKey(key.data, key.len, &tmp);
        std::string name(tmp.name.data, tmp.name.len);
        std::string keyname(tmp.key.data, tmp.key.len);
        if (tmp.type != m_internalType || name != m_hashName) {
            break;
        }

        keys->push_back(keyname);
        vals->push_back(std::string(it.value().data, it.value().len));
        it.next();
    }
}

void THash::hclear(void)
{
    KeyValues result;
    hgetall(&result);
    KeyValues::iterator it = result.begin();
    for (; it != result.end(); ++it) {
        KeyValue& item = *it;
        hdel(item.first);
    }
}


