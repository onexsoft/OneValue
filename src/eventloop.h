#ifndef EVENTLOOPPOOL_H
#define EVENTLOOPPOOL_H

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/event_compat.h>
#include <event2/thread.h>

#include "util/thread.h"

class EventLoop;
class Event
{
public:
    Event(void);
    ~Event(void);

    //Set event param
    void set(EventLoop* loop, evutil_socket_t sock, short flags, event_callback_fn fn, void* arg);

    //Set timeout event
    void setTimer(EventLoop* loop, event_callback_fn fn, void* arg);

    //Active
    void active(int timeout_msec = -1);

    //Remove event from event loop
    void remove(void);

private:
    event m_event;
    friend class EventLoop;
};


class EventLoop
{
public:
    EventLoop(void);
    ~EventLoop(void);

    void exec(void);
    void exit(int timeout = -1);

private:
    event_base* m_event_loop;
    friend class Event;
    EventLoop(const EventLoop&);
    EventLoop& operator=(const EventLoop&);
};


class EventLoopThread : public Thread
{
public:
    EventLoopThread(void);
    ~EventLoopThread(void);

    EventLoop* eventLoop(void) { return &m_loop; }
    void exit(void);

protected:
    virtual void run(void);

public:
    Event m_timeout;
    EventLoop m_loop;
};


class EventLoopThreadPool
{
public:
    enum {
        DefaultThreadCount = 4,
        MaxThreadCount = 32
    };

    EventLoopThreadPool(void);
    ~EventLoopThreadPool(void);

    void start(int size = DefaultThreadCount);
    void stop(void);

    int size(void) const { return m_size; }
    EventLoopThread* thread(int index) const;

private:
    int m_size;
    EventLoopThread* m_threads;
    EventLoopThreadPool(const EventLoopThreadPool&);
    EventLoopThreadPool& operator=(const EventLoopThreadPool&);
};

#endif
