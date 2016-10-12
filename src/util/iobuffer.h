#ifndef IOBUFFER_H
#define IOBUFFER_H

class IOBuffer
{
public:
    struct DirectCopy {
        char* address;
        int maxsize;
    };

    enum { ChunkSize = 1024 * 64 };
    IOBuffer(void);
    IOBuffer(const IOBuffer& rhs);
    ~IOBuffer(void);
    IOBuffer& operator=(const IOBuffer& rhs);

    void reserve(int size);

    void appendFormatString(const char* format, ...);

    template <typename T>
    void appendT(const T& t) { append((char*)&t, sizeof(T)); }
    void append(const char* data, int size = -1);
    void append(const IOBuffer& rhs);
    void clear(void);

    char* data(void) { return m_ptr; }
    const char* data(void) const { return m_ptr; }
    int size(void) const { return m_offset; }

    bool isEmpty(void) const { return (m_offset == 0); }

    //Fast copy
    DirectCopy beginCopy(void);
    void endCopy(int cpsize);

private:
    void appendChunk(int nums);

private:
    int m_capacity;
    int m_offset;
    char m_data[ChunkSize];
    char* m_ptr;
};


#endif
