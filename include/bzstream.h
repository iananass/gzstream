#pragma once

#include <iostream>
#include <fstream>
#include <string.h>
#include <bzlib.h>

namespace bz
{

class streambuf : public std::streambuf
{
private:
    static const int bufferSize = 47 + 256;    // size of data buff
    // totals 512 bytes under g++ for ifstream at the end.

    BZFILE* _out = nullptr;
    FILE* _out_raw = nullptr;
    std::streampos _pos = -1;
    char buffer[bufferSize]; // data buffer
    bool opened = false;             // open/close state of stream
    int mode;               // I/O mode

    int flush_buffer()
    {
        // Separate the writing of the buffer from overflow() and
        // sync() operation.
        int w = pptr() - pbase();
        int err;
        BZ2_bzWrite(&err, _out, pbase(), w);
        if (err != BZ_OK)
            return EOF;
        pbump(-w);
        return w;
    }

public:
    streambuf()
    {
        setp(buffer, buffer + (bufferSize - 1));
        setg(buffer + 4,     // beginning of putback area
             buffer + 4,     // read position
             buffer + 4);    // end position
        // ASSERT: both input & output capabilities will not be used together
    }

    bool is_open() const
    { return opened; }

    streambuf* open(const char* name, int open_mode)
    {
        if (is_open())
            return nullptr;
        mode = open_mode;
        // no append nor read/write mode
        if ((mode & std::ios::ate) || (mode & std::ios::app) || ((mode & std::ios::in) && (mode & std::ios::out)))
            return nullptr;
        char fmode[4];
        if (mode & std::ios::in)
            fmode[0] = 'r';
        else if (mode & std::ios::out)
            fmode[0] = 'w';
        fmode[1] = 'b';
        fmode[2] = '\0';

        _out_raw = fopen(name, fmode);
        if (! _out_raw)
            return nullptr;
        int err;
        if (mode & std::ios::out)
            _out = BZ2_bzWriteOpen(&err, _out_raw, 6, 0, 0);
        else if (mode & std::ios::in)
            _out = BZ2_bzReadOpen(&err, _out_raw, 0, 0, 0, 0);
        if (err != BZ_OK) {
            fclose(_out_raw);
            return nullptr;
        }
        opened = true;
        _pos = 0;
        setp(buffer, buffer + (bufferSize - 1));
        setg(buffer + 4,     // beginning of putback area
             buffer + 4,     // read position
             buffer + 4);    // end position
        return this;
    }

    std::streampos seekg(std::streamoff off, std::ios_base::seekdir way)
    {
        std::streampos cur = -1;
        if (mode & std::ios::in)
        {
            std::streampos pos;
            cur = tellg();
            switch (way)
            {
            case std::ios_base::cur:
                if (cur == -1)
                    return cur;
                pos = cur + off;
                break;
            case std::ios_base::beg:
                pos = off;
                break;
            case std::ios_base::end:
                throw "Method seekg() from end is not available for bzstream buffers";
                break;
            }
            if ((pos < cur) || (cur == -1))
            {
                int err;
                sync();
                BZ2_bzReadClose(&err, _out);
                if (err == BZ_OK)
                {
                    fseek(_out_raw, 0, SEEK_SET);
                    _out = BZ2_bzReadOpen(&err, _out_raw, 0, 0, 0, 0);
                }
                if (err != BZ_OK) {
                  fclose(_out_raw);
                  throw "Method seekg() has failed during seekg(). Now file is closed()";
                }
                _pos = 0;
                cur = 0;
                setg(buffer + 4,     // beginning of putback area
                     buffer + 4,     // read position
                     buffer + 4);    // end position
            }
            while (cur < pos)
            {
              std::streamsize nb = in_avail();
              if (!nb) {
                underflow();
                nb = in_avail();
              }
              if (cur + nb > pos) {
                nb = pos - cur;
              }
              gbump(nb);
              cur += nb;
            }
        }
        return cur;
    }

    std::streampos seekg(std::streampos sp)
    {
        return seekg(sp, std::ios_base::beg);
    }

    std::streampos tellg()
    {
        if (mode & std::ios::in)
            if (_pos >= in_avail())
                return _pos - in_avail();
        return -1;
    }

    streambuf* close()
    {
        if (is_open()) {
            sync();
            opened = false;
            int err;
            if (mode & std::ios::out)
                BZ2_bzWriteClose(&err, _out, 0, nullptr, nullptr);
            else if (mode & std::ios::in)
                BZ2_bzReadClose (&err, _out);

            fclose(_out_raw);
            _out = nullptr;
            _out_raw = nullptr;
            _pos = -1;
            return err == BZ_OK ? this : nullptr;
        }
        return nullptr;
    }

    ~streambuf()
    { close(); }

    virtual int overflow(int c = EOF)
    {
        if (!(mode & std::ios::out) || !opened)
            return EOF;
        if (c != EOF) {
            *pptr() = c;
            pbump(1);
        }
        if (flush_buffer() == EOF)
            return EOF;
        return c;
    }

    virtual int underflow()
    {
        if (gptr() && (gptr() < egptr()))
            return *reinterpret_cast<unsigned char*>( gptr());

        if (!(mode & std::ios::in) || !opened)
            return EOF;
        int n_putback = gptr() - eback();
        if (n_putback > 4)
            n_putback = 4;
        memcpy(buffer + (4 - n_putback), gptr() - n_putback, n_putback);

        int err;
        int num = BZ2_bzRead ( &err, _out,  buffer + 4, bufferSize - 4 );
        if (num <= 0) // ERROR or EOF
        {
            _pos = -1;
            return EOF;
        }

        _pos += num;
        setg(buffer + (4 - n_putback),   // beginning of putback area
             buffer + 4,                 // read position
             buffer + 4 + num);          // end of buffer

        return *reinterpret_cast<unsigned char*>( gptr());
    }

    virtual int sync()
    {
        // Changed to use flush_buffer() instead of overflow( EOF)
        // which caused improper behavior with std::endl and flush(),
        // bug reported by Vincent Ricard.
        if (pptr() && pptr() > pbase()) {
            if (flush_buffer() == EOF)
                return -1;
        }
        return 0;
    }
};

} // gz


