#ifndef STRING_H
#define STRING_H

#include <string.h>

#ifdef WIN32
#pragma warning(disable: 4996)

#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

class String
{
public:
    String(void) : m_str(NULL), m_len(0), m_dup(false) {}
    String(const String& rhs) : m_str(NULL), m_len(0), m_dup(false) { *this = rhs; }
    String(const char* s, int len = -1, bool dup = false) {
        if (len == -1) {
            len = strlen(s);
        }
        m_str = (char*)s;
        m_len = len;
        m_dup = dup;
        if (m_dup) {
            char* p = new char[m_len+1];
            strncpy(p, m_str, m_len);
            p[len] = 0;
            m_str = p;
        }
    }
    String& operator=(const String& rhs) {
        if (this != &rhs) {
            if (m_dup) {
                delete []m_str;
            }
            m_str = rhs.m_str;
            m_len = rhs.m_len;
            m_dup = rhs.m_dup;
            if (m_dup) {
                char* p = new char[m_len+1];
                strncpy(p, m_str, m_len);
                p[m_len] = 0;
                m_str = p;
            }
        }
        return *this;
    }

    ~String(void) {
        if (m_str && m_dup) {
            delete []m_str;
        }
    }

    friend bool operator ==(const String& lhs, const String& rhs) {
        if (lhs.m_len != rhs.m_len) {
            return false;
        }
        return (strncmp(lhs.m_str, rhs.m_str, lhs.m_len) == 0);
    }
    friend bool operator !=(const String& lhs, const String& rhs) {
        return !(lhs == rhs);
    }

    inline const char* data(void) const { return m_str; }
    inline int length(void) const { return m_len; }

private:
    char* m_str;
    unsigned int m_len : 31;
    unsigned int m_dup : 1;
};

#endif
