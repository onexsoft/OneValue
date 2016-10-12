#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

class Logger
{
public:
    enum MsgType {
        Message = 0,
        Warning = 1,
        Error = 2
    };

    Logger(void);
    virtual ~Logger(void);

    //Output handler
    virtual void output(MsgType type, const char* msg);

    //Output function
    static void log(MsgType type, const char* format, ...);

    //Default logger
    static Logger* defaultLogger(void);

    //Set Default logger
    static void setDefaultLogger(Logger* logger);
};



class FileLogger : public Logger
{
public:
    FileLogger(void);
    FileLogger(const char* fileName);
    ~FileLogger(void);

    bool setFileName(const char* fileName);
    const char* fileName(void) const
    { return m_fileName; }

    virtual void output(MsgType type, const char *msg);

private:
    FILE* m_fp;
    char m_fileName[512];
};



#endif
