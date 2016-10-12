#ifndef THREAD_H
#define THREAD_H

class ThreadPrivate;
class Thread
{
public:
    typedef unsigned long long tid_t;

    Thread(void);
    virtual ~Thread(void);

    void start(void);
    void terminate(void);
    bool isRunning(void) const;

    static void sleep(int msec);
    static tid_t currentThreadId(void);

protected:
    virtual void run(void) = 0;

private:
    ThreadPrivate* m_priv;

    friend class WinThread;
    friend class UnixThread;
    Thread(const Thread& rhs);
    Thread& operator=(const Thread& rhs);
};

#endif
