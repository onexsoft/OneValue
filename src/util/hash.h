#ifndef HASH_H
#define HASH_H

#include <unordered_map>

#include "util/string.h"
#include "util/vector.h"

typedef unsigned int (*HashFunc)(const char* s, int len);
unsigned int hashForBytes(const char *key, int len);

struct StringHashFunc {
    unsigned int operator()(const String& str) const {
        return hashForBytes(str.data(), str.length());
    }
};

#define HashTable std::unordered_map

template <typename VAL>
class StringMap : public HashTable<String, VAL, StringHashFunc>
{
public:
};

#endif
