#ifndef REDISPROTO_H
#define REDISPROTO_H

struct Token {
    char* s;
    int len;
};

class RedisProtoParseResult
{
public:
    enum { MaxToken = 1024 };
    enum Type {
        Unknown = 0,    //?????
        Status,         //"+"
        Error,          //"-"
        Integer,        //":"
        Bulk,           //"$"
        MultiBulk       //"*"
    };
    RedisProtoParseResult(void) { reset(); }
    ~RedisProtoParseResult(void) {}

    void reset(void) {
        protoBuff = 0;
        protoBuffLen = 0;
        type = Unknown;
        integer = 0;
        tokenCount = 0;
    }

    char* protoBuff;
    int protoBuffLen;
    int type;
    int integer;
    Token tokens[MaxToken];
    int tokenCount;
};

class RedisProto
{
public:
    enum ParseState {
        ProtoOK = 0,
        ProtoError = -1,
        ProtoIncomplete = -2
    };

    RedisProto(void);
    ~RedisProto(void);

    static void outputProtoString(const char* s, int len);
    static ParseState parse(char* s, int len, RedisProtoParseResult* result);
};

#endif
