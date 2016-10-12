#ifndef OBJECTPOOL_H
#define OBJECTPOOL_H

#include <new>

#include "util/vector.h"

template <typename T>
class ObjectPool
{
public:
    enum { ObjectSize = sizeof(T) };

    ObjectPool(void) {}
    ~ObjectPool(void) { clear(); }

public:
    T* alloc(void) {
        char* ptr = m_pool.pop_back(0);
        if (ptr == 0) {
            ptr = new char[ObjectSize];
        }
        if (ptr) {
            return new (ptr) T;
        }
        return 0;
    }

    void free(T* object) {
        char* ptr = (char*)object;
        if (ptr) {
            object->~T();
            m_pool.push_back(ptr);
        }
    }

    void clear(void) {
        for (int i = 0; i < m_pool.size(); ++i) {
            delete []m_pool.at(i);
        }
        m_pool.clear();
    }

private:
    Vector<char*> m_pool;

private:
    ObjectPool(const ObjectPool&);
    ObjectPool& operator=(const ObjectPool&);
};

#endif
