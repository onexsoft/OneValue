#include <time.h>
#include <stdarg.h>
#include <memory.h>

#include "util/string.h"
#include "logger.h"

static Logger _stdoutput;
Logger* _defaultLogger = &_stdoutput;

static const char* msg_type_text[] =
{
    "Message",
    "Warning",
    "Error"
};

Logger::Logger(void)
{
}

Logger::~Logger(void)
{
}

void Logger::output(Logger::MsgType type, const char *msg)
{
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    printf("[%d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
           lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
           lt->tm_hour, lt->tm_min, lt->tm_sec, msg_type_text[type], msg);
}

void Logger::log(Logger::MsgType type, const char *format, ...)
{
    if (!format || !_defaultLogger) {
        return;
    }

    char buffer[10240];
    va_list marker;
    va_start(marker, format);
    vsprintf(buffer, format, marker);
    va_end(marker);

    _defaultLogger->output(type, buffer);
}

Logger *Logger::defaultLogger(void)
{
    return _defaultLogger;
}

void Logger::setDefaultLogger(Logger *logger)
{
    _defaultLogger = logger;
}



FileLogger::FileLogger(void)
{
    m_fp = NULL;
    memset(m_fileName, 0, sizeof(m_fileName));
}

FileLogger::FileLogger(const char *fileName)
{
    setFileName(fileName);
}

FileLogger::~FileLogger(void)
{
    if (m_fp) {
        fclose(m_fp);
    }
}

bool FileLogger::setFileName(const char *fileName)
{
    FILE* fp = fopen(fileName, "a+");
    if (!fp) {
        return false;
    }

    strcpy(m_fileName, fileName);
    if (m_fp) {
        fclose(m_fp);
    }
    m_fp = fp;
    return true;
}

void FileLogger::output(Logger::MsgType type, const char *msg)
{
    time_t t = time(NULL);
    tm* lt = localtime(&t);

    fprintf(m_fp, "[%d-%02d-%02d %02d:%02d:%02d] %s: %s\n",
            lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday,
            lt->tm_hour, lt->tm_min, lt->tm_sec, msg_type_text[type], msg);
    fflush(m_fp);
}
