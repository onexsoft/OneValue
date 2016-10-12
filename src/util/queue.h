#ifndef QUEUE_H
#define QUEUE_H

#include "util/objectpool.h"

template <typename T>
class Queue
{
public:
    struct Node {
        T item;
        Node* next;
    };

    Queue(void) : m_entry(NULL), m_tail(NULL) {}
    ~Queue(void) { clear(); }

    void append(const T& object) {
        if (m_tail) {
            m_tail->next = m_objectPool.alloc();
            if (m_tail->next) {
                m_tail->next->item = object;
                m_tail = m_tail->next;
                m_tail->next = NULL;
            }
        } else {
            m_entry = m_objectPool.alloc();
            m_entry->item = object;
            m_entry->next = NULL;
            m_tail = m_entry;
        }
    }

    T take(const T& defaultVal) {
        Node* node = m_entry;
        if (!node) {
            return defaultVal;
        }
        T ret = node->item;
        m_entry = m_entry->next;
        if (m_entry == NULL) {
            m_tail = NULL;
        }
        m_objectPool.free(node);
        return ret;
    }

    void clear(void) {
        for (Node* node = m_entry; node != NULL;) {
            Node* next = node->next;
            m_objectPool.free(node);
            node = next;
        }
        m_entry = NULL;
        m_tail = NULL;
    }

    bool isEmpty(void) const
    { return (m_entry == 0); }

private:
    Node* m_entry;
    Node* m_tail;
    ObjectPool<Node> m_objectPool;
};

#endif
