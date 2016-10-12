#include "util/hash.h"

unsigned int hashForBytes(const char *key, int len)
{
    /*
    int seed = 131;
    unsigned int hash = 0;
    const char* c = key;
    int i = 0;
    while (i++ < len) {
        hash = hash * seed + (*c++);
    }
    return (hash & 0x7FFFFFFF);
    */

    const unsigned int m = 0x5bd1e995;
    const int r = 24;
    const int seed = 131;

    unsigned int h = seed ^ len;

    const unsigned char *data = (const unsigned char *)key;

    while(len >= 4) {
        unsigned int k = *(unsigned int*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0]; h *= m;
    };


    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int)h;
}
