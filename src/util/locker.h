#ifndef LOCKER_H
#define LOCKER_H

#ifdef WIN32
typedef void* HANDLE;
#else
#include <pthread.h>
#endif

class Mutex
{
public:
    Mutex(void);
    ~Mutex(void);

    void lock(void);
    void unlock(void);

#ifdef WIN32
    HANDLE m_hMutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

class SpinLocker
{
public:
    SpinLocker(void);
    ~SpinLocker(void);

    void lock(void);
    void unlock(void);

private:
#ifndef WIN32
    pthread_spinlock_t m_spinlock;
#else
    Mutex m_mutex;
#endif
};

#endif
