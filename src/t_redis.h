#ifndef T_REDIS_H
#define T_REDIS_H

#include <string>
#include <list>

#include "util/string.h"

enum {
    T_KV = 100,
    T_List,
    T_ListElement,
    T_Set,
    T_ZSet,
    T_Hash,
    T_Ttl
};

typedef std::list<std::string> stringlist;

class TRedisHelper
{
public:
    static bool isInteger(const char* buff, int len);
    static bool isInteger(const std::string& s)
    { return isInteger(s.data(), s.size()); }
    static bool isDouble(const char* buff, int len);
    static bool isDouble(const std::string& s)
    { return isDouble(s.data(), s.size()); }
    static int doubleToString(char* buff, double f, bool clearZero = true);
};

#endif
