#ifndef DBCOPY_H
#define DBCOPY_H

#include "util/tcpsocket.h"
#include "util/iobuffer.h"

#include "leveldb.h"

class DBCopy
{
public:
    DBCopy(void);
    ~DBCopy(void);

    void setMaxPipeline(int n);
    int maxPipeline(void) const { return m_maxPipeline; }
    bool start(LeveldbCluster* src, const HostAddress& dest);

private:
    void makeCommand(IOBuffer* buff, const XObject& key, const XObject& value);

private:
    int m_maxPipeline;
};

#endif
