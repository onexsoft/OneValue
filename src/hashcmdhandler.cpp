#include "hashcmdhandler.h"
#include <stdlib.h>
#include <string>
#include "redisproxy.h"
#include "t_hash.h"
#include "t_redis.h"



//  hset user.1 tech lisi
void onHsetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);

    char* _key = parseResult.tokens[2].s;
    std::string key(_key, parseResult.tokens[2].len);

    char* _value = parseResult.tokens[3].s;
    std::string value(_value, parseResult.tokens[3].len);

    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    if (t_hash.hset(key, value)) {
        packet->sendBuff.append(":1\r\n");
    } else {
        packet->sendBuff.append(":0\r\n");
    }

    packet->setFinishedState(ClientPacket::RequestFinished);
}

// hget user.1 name
void onHgetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);

    char* _key = parseResult.tokens[2].s;
    std::string key(_key, parseResult.tokens[2].len);

    THash t_hash(packet->proxy()->leveldbCluster(), hashName);

    IOBuffer& reply = packet->sendBuff;
    std::string value;
    if (t_hash.hget(key, &value)) {
        reply.appendFormatString("$%d\r\n", value.size());
        reply.append(value.data(), value.size());
        reply.append("\r\n");
    } else {
        reply.append("$-1\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


// hmget user.1 name age fri tech
void onHmgetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenCnt = parseResult.tokenCount;
    if (tokenCnt < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);

    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;

    reply.appendFormatString("*%d\r\n", tokenCnt - 2);

    for (int i = 2; i < tokenCnt; ++i) {
        char* _field = parseResult.tokens[i].s;
        std::string field(_field, parseResult.tokens[i].len);
        std::string val;
        if (!t_hash.hget(field, &val)) {
            reply.appendFormatString("$-1\r\n");
            continue;
        }
        reply.appendFormatString("$%d\r\n", val.length());
        reply.append(val.data(), val.length());
        reply.append("\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}



// hgetall user1
void onHgetAllCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;

    KeyValues result;
    t_hash.hgetall(&result);
    reply.appendFormatString("*%d\r\n", result.size() * 2);
    KeyValues::iterator it = result.begin();
    for (; it != result.end(); ++it) {
        KeyValue& item = *it;
        reply.appendFormatString("$%d\r\n%s\r\n", item.first.length(), item.first.c_str());
        reply.appendFormatString("$%d\r\n%s\r\n", item.second.length(), item.second.c_str());
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}

// hkeys   user1
void onHkeysCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;

    KeyValues result;
    t_hash.hgetall(&result);
    reply.appendFormatString("*%d\r\n", result.size());
    KeyValues::iterator it = result.begin();
    for (; it != result.end(); ++it) {
        KeyValue& item = *it;
        reply.appendFormatString("$%d\r\n%s\r\n", item.first.length(), item.first.c_str());
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


//  hvals user1
void onHvalsCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;

    KeyValues result;
    t_hash.hgetall(&result);
    reply.appendFormatString("*%d\r\n", result.size());
    KeyValues::iterator it = result.begin();
    for (; it != result.end(); ++it) {
        KeyValue& item = *it;
        reply.appendFormatString("$%d\r\n%s\r\n", item.second.length(), item.second.c_str());
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


// hexists user.1 name
void onHexistsCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    char* _field = parseResult.tokens[2].s;
    std::string field(_field , parseResult.tokens[2].len);

    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;
    if (t_hash.hexists(field)) {
        reply.appendFormatString(":1\r\n");
    } else {
        reply.appendFormatString(":0\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


//  hincrby  user1 age 1
void onHincrbyCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);

    char* _field = parseResult.tokens[2].s;
    std::string field(_field, parseResult.tokens[2].len);

    char* _incrby = parseResult.tokens[3].s;
    std::string incrby(_incrby, parseResult.tokens[3].len);
    IOBuffer& reply = packet->sendBuff;
    if (!TRedisHelper::isInteger(incrby)) {
        reply.appendFormatString("-ERR value is not an integer or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }

    long incrby_ = atoi(incrby.c_str());

    char buf[128] = {0};
    if (t_hash.hexists(field)) {
        std::string num;
        t_hash.hget(field, &num);
        if (!TRedisHelper::isInteger(num)) {
            reply.appendFormatString("-ERR hash value is not an integer\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        long needSet = atoi(num.c_str()) + incrby_;
        sprintf(buf, "%ld", needSet);
    } else {
        sprintf(buf, "%ld", incrby_);
    }
    std::string _value = buf;

    if (!t_hash.hset(field, _value)) {
        reply.appendFormatString("-ERR hset error\r\n");
    } else {
        reply.appendFormatString(":");
        reply.append(buf, strlen(buf));
        reply.append("\r\n");
    }

    packet->setFinishedState(ClientPacket::RequestFinished);
}


void onHincrbyFloatCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);

    char* _field = parseResult.tokens[2].s;
    std::string field(_field, parseResult.tokens[2].len);
    char* _incrby = parseResult.tokens[3].s;
    std::string incrby(_incrby, parseResult.tokens[3].len);
    IOBuffer& reply = packet->sendBuff;
    if (!TRedisHelper::isDouble(incrby)) {
        reply.appendFormatString("-ERR value is not an float or out of range\r\n");
        packet->setFinishedState(ClientPacket::RequestFinished);
        return;
    }
    double f_incrby = atof(incrby.c_str());

    char buf[128] = {0};
    if (t_hash.hexists(field)) {
        std::string num;
        t_hash.hget(field, &num);
        if (!TRedisHelper::isDouble(num)) {
            reply.appendFormatString("-ERR hash value is not an float\r\n");
            packet->setFinishedState(ClientPacket::RequestFinished);
            return;
        }
        double needSet = atof(num.c_str()) + f_incrby;
        TRedisHelper::doubleToString(buf, needSet);
    } else {
        TRedisHelper::doubleToString(buf, f_incrby);
    }
    std::string _value = buf;

    if (!t_hash.hset(field, _value)) {
        reply.appendFormatString("-ERR hset error\r\n");
    } else {
        reply.appendFormatString("*1\r\n");
        reply.appendFormatString("$%d\r\n", strlen(buf));
        reply.append(buf, strlen(buf));
        reply.append("\r\n");
    }

    packet->setFinishedState(ClientPacket::RequestFinished);
}



// hdel user1 field1 field2 ...
void onHdelCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut < 3) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);

    int delNum = 0;
    for (int i = 2; i < tokenConut; ++i) {
        char* _field = parseResult.tokens[i].s;
        std::string field(_field, parseResult.tokens[i].len);
        if (t_hash.hdel(field)) {
            ++delNum;
        }
    }

    IOBuffer& reply = packet->sendBuff;
    reply.appendFormatString(":%d\r\n", delNum);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


//  hlen user1
void onHlenCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    int fieldNum = t_hash.hlen();
    IOBuffer& reply = packet->sendBuff;
    reply.appendFormatString(":%d\r\n", fieldNum);
    packet->setFinishedState(ClientPacket::RequestFinished);
}


//  hmset user.2 name niuer age 34
void onHmsetCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    int tokenConut = parseResult.tokenCount;
    if (tokenConut % 2 != 0) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    IOBuffer& reply = packet->sendBuff;
    for (int i = 2; i < tokenConut; i += 2) {
        char* _key = parseResult.tokens[i].s;
        std::string key(_key, parseResult.tokens[i].len);
        char* _value = parseResult.tokens[i + 1].s;
        std::string value(_value, parseResult.tokens[i + 1].len);
        t_hash.hset(key, value);
    }

    reply.appendFormatString("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


// hsetnx user.1 name lisi
void onHsetnxCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 4) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }
    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);

    char* _key = parseResult.tokens[2].s;
    std::string key(_key, parseResult.tokens[2].len);
    char* _value = parseResult.tokens[3].s;
    std::string value(_value, parseResult.tokens[3].len);

    IOBuffer& reply = packet->sendBuff;
    if (!t_hash.hexists(key)) {
        t_hash.hset(key, value);
        reply.appendFormatString(":1\r\n");
    } else {
        reply.appendFormatString(":0\r\n");
    }
    packet->setFinishedState(ClientPacket::RequestFinished);
}


// hclear key
void onHClearCommand(ClientPacket* packet, void*)
{
    RedisProtoParseResult& parseResult = packet->recvParseResult;
    if (parseResult.tokenCount != 2) {
        packet->setFinishedState(ClientPacket::WrongNumberOfArguments);
        return;
    }

    char* _hashName = parseResult.tokens[1].s;
    std::string hashName(_hashName, parseResult.tokens[1].len);
    THash t_hash(packet->proxy()->leveldbCluster(), hashName);
    t_hash.hclear();

    packet->sendBuff.appendFormatString("+OK\r\n");
    packet->setFinishedState(ClientPacket::RequestFinished);
}


