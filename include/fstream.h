#pragma once

#include <iostream>
#include <fstream>

namespace compression {

template <typename StreamBuf>
class streambase : virtual public std::ios
{
public:
    using streambuf = StreamBuf;

    streambase()
    { init(&buf); }

    streambase(const char* name, int open_mode)
    {
        init(&buf);
        open(name, open_mode);
    }

    ~streambase()
    {
        buf.close();
    }

    void open(const char* name, int open_mode)
    {
        if (!buf.open(name, open_mode))
            clear(rdstate() | std::ios::badbit);
    }

    void close()
    {
        if (buf.is_open())
            if (!buf.close())
                clear(rdstate() | std::ios::badbit);
    }

    streambuf* rdbuf()
    { return &buf; }

protected:
    streambuf buf;
};

template <typename StreamBuf>
class ifstream : public streambase<StreamBuf>, public std::istream
{
public:
    ifstream() : std::istream(&this->buf)
    {}

    ifstream(const char* name, int open_mode = std::ios::in)
        : streambase<StreamBuf>(name, open_mode)
        , std::istream(&this->buf)
    {}

    typename streambase<StreamBuf>::streambuf * rdbuf()
    { return streambase<StreamBuf>::rdbuf(); }

    void open(const char* name, int open_mode = std::ios::in)
    {
        streambase<StreamBuf>::open(name, open_mode);
    }
};

template <typename StreamBuf>
class ofstream : public streambase<StreamBuf>, public std::ostream
{
public:
    ofstream()
        : std::ostream(&this->buf)
    {}

    ofstream(const char* name, int mode = std::ios::out)
        : streambase<StreamBuf>(name, mode)
        , std::ostream(&this->buf)
    {}

    typename streambase<StreamBuf>::streambuf* rdbuf()
    { return streambase<StreamBuf>::rdbuf(); }

    void open(const char* name, int open_mode = std::ios::out)
    {
        streambase<StreamBuf>::open(name, open_mode);
    }
};

template <typename StreamBuf>
ifstream<StreamBuf> &getline(ifstream<StreamBuf> &ifs, std::string &s, const char delim = '\n')
{
    char c;
    s.clear();
    while (ifs.get(c) && (c != delim)) {
        s += c;
    }
    return ifs;
}

template <typename StreamBuf>
ifstream<StreamBuf> &ignore(ifstream<StreamBuf> &ifs, size_t limit, const char delim = '\n')
{
    char c;
    while (limit-- && ifs.get(c) && (c != delim));
    return ifs;
}

}
