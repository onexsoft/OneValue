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
#include "redisproxy.h"
#include "command.h"
#include "cmdhandler.h"
#include "zsetcmdhandler.h"
#include "hashcmdhandler.h"
#include "listcmdhandler.h"

static RedisCommand _redisCommand[] = {
    {"APPEND", 6, RedisCommand::APPEND, onAppendCommand, NULL},
    {"DECR", 4, RedisCommand::DECR, onDecrCommand, NULL},
    {"DECRBY", 6, RedisCommand::DECRBY, onDecrByCommand, NULL},
    {"GETRANGE", 8, RedisCommand::GETRANGE, onGetRangeCommand, NULL},
    {"GETSET", 6, RedisCommand::GETSET, onGetSetCommand, NULL},
    {"INCR", 4, RedisCommand::INCR, onIncrCommand, NULL},
    {"INCRBY", 6, RedisCommand::INCRBY, onIncrByCommand, NULL},
    {"INCRBYFLOAT", 11, RedisCommand::INCRBYFLOAT, onIncrByFloatCommand, NULL},
    {"MGET", 4, RedisCommand::MGET, onMGetCommand, NULL},
    {"MSET", 4, RedisCommand::MSET, onMSetCommand, NULL},
    {"EXISTS", 6, RedisCommand::EXISTS, onExistsCommand, NULL},
    {"MSETNX", 6, RedisCommand::MSETNX, onMSetNXCommand, NULL},
    //{"PSETEX", 6, RedisCommand::PSETEX, onPSetEXCommand, NULL},
    {"SETEX", 5, RedisCommand::SETEX, onSetEXCommand, NULL},
    {"SETNX", 5, RedisCommand::SETNX, onSetNXCommand, NULL},
    {"SETRANGE", 8, RedisCommand::SETRANGE, onSetRangeCommand, NULL},
    {"STRLEN", 6, RedisCommand::STRLEN, onStrlenCommand, NULL},

    {"GET", 3, RedisCommand::GET, onGetCommand, NULL},
    {"SET", 3, RedisCommand::SET, onSetCommand, NULL},
    {"DEL", 3, RedisCommand::DEL, onDelCommand, NULL},
    {"EXPIRE", 6, RedisCommand::EXPIRE, onExpireCommand, NULL},

    {"ZADD", 4, RedisCommand::ZADD, onZAddCommand, NULL},
    {"ZREM", 4, RedisCommand::ZREM, onZRemCommand, NULL},
    {"ZINCRBY", 7, RedisCommand::ZINCRBY, onZIncrbyCommand, NULL},
    {"ZRANK", 5, RedisCommand::ZRANK, onZRankCommand, NULL},
    {"ZREVRANK", 8, RedisCommand::ZREVRANK, onZRevRankCommand, NULL},
    {"ZRANGE", 6, RedisCommand::ZRANGE, onZRangeCommand, NULL},
    {"ZREVRANGE", 9, RedisCommand::ZREVRANGE, onZRevRangeCommand, NULL},
    {"ZRANGEBYSCORE", 13, RedisCommand::ZRANGEBYSCORE, onZRangeByScoreCommand, NULL},
    {"ZREVRANGEBYSCORE", 16, RedisCommand::ZREVRANGEBYSCORE, onZRevRangeByScoreCommand, NULL},
    {"ZCOUNT", 6, RedisCommand::ZCOUNT, onZCountCommand, NULL},
    {"ZCARD", 5, RedisCommand::ZCARD, onZCardCommand, NULL},
    {"ZSCORE", 6, RedisCommand::ZSCORE, onZSCoreCommand, NULL},
    {"ZREMRANGEBYRANK", 15, RedisCommand::ZREMRANGEBYRANK, onZRemRangeByRankCommand, NULL},
    {"ZREMRANGEBYSCORE", 16, RedisCommand::ZREMRANGEBYSCORE, onZRemRangeByScoreCommand, NULL},
    {"ZCLEAR", 6, RedisCommand::ZCLEAR, onZClear, NULL},

    {"SADD", 4, RedisCommand::SADD, onSAddCommand, NULL},
    {"SCARD", 5, RedisCommand::SCARD, onSCardCommand, NULL},
    {"SDIFF", 5, RedisCommand::SDIFF, onSDiffCommand, NULL},
    {"SDIFFSTORE", 10, RedisCommand::SDIFFSTORE, onSDiffStoreCommand, NULL},
    {"SINTER", 6, RedisCommand::SINTER, onSInterCommand, NULL},
    {"SINTERSTORE", 11, RedisCommand::SINTERSTORE, onSInterStoreCommand, NULL},
    {"SISMEMBER", 9, RedisCommand::SISMEMBER, onSIsMemberCommand, NULL},
    {"SMEMBERS", 8, RedisCommand::SMEMBERS, onSMembersCommand, NULL},
    {"SMOVE", 5, RedisCommand::SMOVE, onSMoveCommand, NULL},
    {"SPOP", 4, RedisCommand::SPOP, onSPopCommand, NULL},
    {"SRANDMEMBER", 11, RedisCommand::SRANDMEMBER, onSRandMember, NULL},
    {"SREM", 4, RedisCommand::SREM, onSRemCommand, NULL},
    {"SUNION", 6, RedisCommand::SUNION, onSUnionCommand, NULL},
    {"SUNIONSTORE", 11, RedisCommand::SUNIONSTORE, onSUnionStoreCommand, NULL},
    {"SCLEAR", 6, RedisCommand::SCLEAR, onSClearCommand, NULL},

    {"HSET", 4, RedisCommand::HSET, onHsetCommand, NULL},
    {"HGET", 4, RedisCommand::HGET, onHgetCommand, NULL},
    {"HMGET", 5, RedisCommand::HMGET, onHmgetCommand, NULL},
    {"HGETALL", 7, RedisCommand::HGETALL, onHgetAllCommand, NULL},
    {"HEXISTS", 7, RedisCommand::HEXISTS, onHexistsCommand, NULL},
    {"HKEYS", 5, RedisCommand::HKEYS, onHkeysCommand, NULL},
    {"HVALS", 5, RedisCommand::HVALS, onHvalsCommand, NULL},
    {"HINCRBY", 7, RedisCommand::HINCRBY, onHincrbyCommand, NULL},
    {"HINCRBYFLOAT", 12, RedisCommand::HINCRBYFLOAT, onHincrbyFloatCommand, NULL},
    {"HDEL", 4, RedisCommand::HDEL, onHdelCommand, NULL},
    {"HLEN", 4, RedisCommand::HLEN, onHlenCommand, NULL},
    {"HMSET", 5, RedisCommand::HMSET, onHmsetCommand, NULL},
    {"HSETNX", 6, RedisCommand::HSETNX, onHsetnxCommand, NULL},
    {"HCLEAR", 6, RedisCommand::HCLEAR, onHClearCommand, NULL},

    {"LINDEX", 6, RedisCommand::LINDEX, onLindexCommand, NULL},
    {"LINSERT", 7, RedisCommand::LINSERT, onLinsertCommand, NULL},
    {"LLEN", 4, RedisCommand::LLEN, onLlenCommand, NULL},
    {"LPOP", 4, RedisCommand::LPOP, onLpopCommand, NULL},
    {"LPUSH", 5, RedisCommand::LPUSH, onLpushCommand, NULL},
    {"LPUSHX", 6, RedisCommand::LPUSHX, onLpushxCommand, NULL},
    {"LRANGE", 6, RedisCommand::LRANGE, onLRangeCommand, NULL},
    {"LREM", 4, RedisCommand::LREM, onLRemCommand, NULL},
    {"LSET", 4, RedisCommand::LSET, onLSetCommand, NULL},
    {"LTRIM", 5, RedisCommand::LTRIM, onLTrimCommand, NULL},
    {"RPOP", 4, RedisCommand::RPOP, onRPopCommand, NULL},
    {"RPUSH", 5, RedisCommand::RPUSH, onRPushCommand, NULL},
    {"RPUSHX", 6, RedisCommand::RPUSHX, onRpushxCommand, NULL},
    {"RPOPLPUSH", 9, RedisCommand::RPOPLPUSH, onRPopLPushCommand, NULL},
    {"LCLEAR", 6, RedisCommand::LCLEAR, onLClearCommand, NULL},

    {"PING", 4, RedisCommand::PING, onPingCommand, NULL},
    {"SHOWCMD", 7, -1, onShowCommand, NULL},
    {"RAWSET", 6, RedisCommand::PrivType, onRawSetCommand, NULL},
    {"FLUSHDB", 7, RedisCommand::PrivType, onFlushdbCommand, NULL}
};

const char *RedisCommand::commandName(int type)
{
    if (type >= 0 && type < RedisCommand::CMD_COUNT) {
        return _redisCommand[type].name;
    }
    return "";
}



RedisCommandTable::RedisCommandTable(void)
{
}

RedisCommandTable::~RedisCommandTable(void)
{
    std::list<RedisCommand*>::iterator it = m_cmds.begin();
    for (; it != m_cmds.end(); ++it) {
        delete *it;
    }
}

RedisCommandTable* RedisCommandTable::instance(void)
{
    static RedisCommandTable* table = NULL;
    if (!table) {
        table = new RedisCommandTable;
        table->registerCommand(_redisCommand, sizeof(_redisCommand) / sizeof(RedisCommand));
    }
    return table;
}

int RedisCommandTable::registerCommand(RedisCommand *cmd, int cnt)
{
    int succeed = 0;
    for (int i = 0; i < cnt; ++i) {
        char* name = cmd[i].name;
        int len = cmd[i].len;

        for (int j = 0; j < len; ++j) {
            if (name[j] >= 'a' && name[j] <= 'z') {
                name[j] += ('A' - 'a');
            }
        }

        RedisCommand* val = new RedisCommand(cmd[i]);
        m_cmds.push_back(val);
        m_cmdMap.insert(StringMap<RedisCommand*>::value_type(String(name, len, true), val));
        ++succeed;
    }
    return succeed;
}

bool RedisCommandTable::registerCommand(const char *name, int type, CommandHandler handler, void *arg)
{
    size_t len = strlen(name);
    RedisCommand cmd;
    if (len > sizeof(cmd.name)-1) {
        return false;
    }
    strcpy(cmd.name, name);
    cmd.len = len;
    cmd.type = type;
    cmd.handler = handler;
    cmd.arg = arg;
    return (registerCommand(&cmd, 1) == 1);
}

RedisCommand *RedisCommandTable::findCommand(const char *cmd, int len) const
{
    //to upper
    char* p = (char*)cmd;
    for (int i = 0; i < len; ++i) {
        if (p[i] >= 'a' && p[i] <= 'z') {
            p[i] += ('A' - 'a');
        }
    }

    StringMap<RedisCommand*>::const_iterator it = m_cmdMap.find(String(cmd, len));
    if (it != m_cmdMap.cend()) {
        return it->second;
    } else {
        return NULL;
    }
}
