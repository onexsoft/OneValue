#include "util/logger.h"
#include "eventloop.h"

Event::Event(void)
{
}

Event::~Event(void)
{
}

void Event::set(EventLoop *loop, evutil_socket_t sock, short flags, event_callback_fn fn, void *arg)
{
    event_assign(&m_event, loop->m_event_loop, sock, flags, fn, arg);
}

void Event::setTimer(EventLoop *loop, event_callback_fn fn, void *arg)
{
    evtimer_assign(&m_event, loop->m_event_loop, fn, arg);
}

void Event::active(int timeout)
{
    if (timeout != -1) {
        timeval val;
        val.tv_sec = (timeout / 1000);
        val.tv_usec = (timeout - val.tv_sec * 1000) * 1000;
        event_add(&m_event, &val);
    } else {
        event_add(&m_event, NULL);
    }
}

void Event::remove(void)
{
    event_del(&m_event);
}



EventLoop::EventLoop(void)
{
    static bool b = false;
    if (!b) {
#ifdef WIN32
        evthread_use_windows_threads();
#else
        evthread_use_pthreads();
#endif
        b = true;
    }
    m_event_loop = event_base_new();
}

EventLoop::~EventLoop(void)
{
    if (m_event_loop) {
        event_base_free(m_event_loop);
    }
}

void EventLoop::exec(void)
{
    if (m_event_loop) {
        if (event_base_dispatch(m_event_loop) < 0) {
            Logger::log(Logger::Error, "EventLoop::exec: event_base_dispatch() failed");
        }
    }
}

void EventLoop::exit(int timeout)
{
    if (m_event_loop) {
        timeval* p = NULL;
        timeval val;
        if (timeout != -1) {
            val.tv_sec = (timeout / 1000);
            val.tv_usec = (timeout - val.tv_sec * 1000) * 1000;
            p = &val;
        }
        if (event_base_loopexit(m_event_loop, p) < 0) {
            Logger::log(Logger::Error, "EventLoop::exit: event_base_loopexit() failed");
        }
    }
}



EventLoopThread::EventLoopThread(void)
{
}

EventLoopThread::~EventLoopThread(void)
{
}

void EventLoopThread::exit(void)
{
    m_timeout.remove();
    m_loop.exit();
}

void emptyCallBack(evutil_socket_t, short, void*)
{
}

void EventLoopThread::run(void)
{
    m_timeout.set(&m_loop, -1, EV_PERSIST | EV_TIMEOUT, emptyCallBack, this);
    m_timeout.active(60000);
    m_loop.exec();
}


EventLoopThreadPool::EventLoopThreadPool(void)
{
    m_size = 0;
    m_threads = NULL;
}

EventLoopThreadPool::~EventLoopThreadPool(void)
{
    stop();
}

void EventLoopThreadPool::start(int size)
{
    stop();

    m_size = size;
    if (m_size <= 0 || m_size > MaxThreadCount) {
        Logger::log(Logger::Warning, "EventLoopThreadPool::start: size out of range. use default size");
        m_size = DefaultThreadCount;
    }

    Logger::log(Logger::Message, "Create the thread pool...");
    m_threads = new EventLoopThread[m_size];
    for (int i = 0; i < m_size; ++i) {
        m_threads[i].start();
    }

    Thread::sleep(100);
    Logger::log(Logger::Message, "Thread pool created. size: %d", m_size);
}

void EventLoopThreadPool::stop(void)
{
    if (m_threads) {
        Logger::log(Logger::Message, "Destroy thread pool...");
        for (int i = 0; i < m_size; ++i) {
            m_threads[i].exit();
        }
        delete []m_threads;
        m_threads = NULL;
        m_size = 0;
        Logger::log(Logger::Message, "Thread pool destroyed");
    }
}

EventLoopThread *EventLoopThreadPool::thread(int index) const
{
    if ((index >= 0) && (index < m_size)) {
        return &m_threads[index];
    }
    return NULL;
}



