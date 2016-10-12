#ifndef COMMAND_H
#define COMMAND_H

#include <list>

#include "util/hash.h"

class ClientPacket;
typedef void (*CommandHandler)(ClientPacket*, void*);

class RedisCommand
{
public:
    enum {
        PrivType = -1191210
    };
    enum Type {
        APPEND, DECR, DECRBY, GETRANGE, GETSET, INCR, INCRBY, INCRBYFLOAT,
        MGET, MSET, EXISTS, MSETNX, PSETEX, SETEX, SETNX, SETRANGE, STRLEN,
        GET, SET, DEL, EXPIRE,
        ZADD, ZREM, ZINCRBY, ZRANK, ZREVRANK, ZRANGE,
        ZREVRANGE, ZRANGEBYSCORE, ZREVRANGEBYSCORE, ZCOUNT,
        ZCARD, ZSCORE, ZREMRANGEBYRANK, ZREMRANGEBYSCORE, ZCLEAR,
        SADD, SCARD, SDIFF, SDIFFSTORE, SINTER, SINTERSTORE, SISMEMBER,
        SMEMBERS, SMOVE, SPOP, SRANDMEMBER, SREM, SUNION, SUNIONSTORE, SCLEAR,
        HSET, HGET, HMGET, HGETALL, HEXISTS, HKEYS, HVALS,
        HINCRBY, HINCRBYFLOAT, HDEL, HLEN, HMSET, HSETNX, HCLEAR,
        LINDEX, LINSERT, LLEN, LPOP, LPUSH, LPUSHX, LRANGE,
        LREM, LSET, LTRIM, RPOP, RPUSH, RPUSHX, RPOPLPUSH, LCLEAR,
        PING
    };
    enum {
        CMD_COUNT = PING+1
    };

public:
    static const char* commandName(int type);

public:
    char name[32];
    int len;
    int type;
    CommandHandler handler;
    void* arg;
};

class RedisCommandTable
{
public:
    RedisCommandTable(void);
    ~RedisCommandTable(void);

    static RedisCommandTable* instance(void);

    int registerCommand(RedisCommand* cmd, int cnt);
    bool registerCommand(const char* name, int type, CommandHandler handler, void* arg);
    RedisCommand* findCommand(const char* cmd, int len) const;

    const std::list<RedisCommand*>& commands(void) const { return m_cmds; }

private:
    StringMap<RedisCommand*> m_cmdMap;
    std::list<RedisCommand*> m_cmds;
};

#endif
