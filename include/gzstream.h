#pragma once

#include <iostream>
#include <fstream>
#include <string.h>
#include <zlib.h>

namespace gz
{

// ----------------------------------------------------------------------------
// Internal classes to implement gzstream. See below for user classes.
// ----------------------------------------------------------------------------

class streambuf : public std::streambuf
{
private:
    static const int bufferSize = 47 + 256;    // size of data buff
    // totals 512 bytes under g++ for ifstream at the end.

    gzFile file;               // file handle for compressed file
    char buffer[bufferSize]; // data buffer
    bool opened = false;             // open/close state of stream
    int mode;               // I/O mode

    int flush_buffer()
    {
        // Separate the writing of the buffer from overflow() and
        // sync() operation.
        int w = pptr() - pbase();
        if (gzwrite(file, pbase(), w) != w)
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
            return (streambuf*) 0;
        mode = open_mode;
        // no append nor read/write mode
        if ((mode & std::ios::ate) || (mode & std::ios::app)
            || ((mode & std::ios::in) && (mode & std::ios::out)))
            return (streambuf*) 0;
        char fmode[10];
        char* fmodeptr = fmode;
        if (mode & std::ios::in)
            *fmodeptr++ = 'r';
        else if (mode & std::ios::out)
            *fmodeptr++ = 'w';
        *fmodeptr++ = 'b';
        *fmodeptr = '\0';
        file = gzopen(name, fmode);
        if (file == 0)
            return (streambuf*) 0;
        opened = true;
        setp(buffer, buffer + (bufferSize - 1));
        setg(buffer + 4,     // beginning of putback area
             buffer + 4,     // read position
             buffer + 4);    // end position
        return this;
    }

    streambuf* close()
    {
        if (is_open()) {
            sync();
            opened = false;
            if (gzclose(file) == Z_OK)
                return this;
        }
        return (streambuf*) 0;
    }

    ~streambuf()
    { close(); }

    virtual int overflow(int c = EOF)
    { // used for output buffer only
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
    { // used for input buffer only
        if (gptr() && (gptr() < egptr()))
            return *reinterpret_cast<unsigned char*>( gptr());

        if (!(mode & std::ios::in) || !opened)
            return EOF;
        // Josuttis' implementation of inbuf
        int n_putback = gptr() - eback();
        if (n_putback > 4)
            n_putback = 4;
        memcpy(buffer + (4 - n_putback), gptr() - n_putback, n_putback);

        int num = gzread(file, buffer + 4, bufferSize - 4);
        if (num <= 0) // ERROR or EOF
            return EOF;

        // reset buffer pointers
        setg(buffer + (4 - n_putback),   // beginning of putback area
             buffer + 4,                 // read position
             buffer + 4 + num);          // end of buffer

        // return next character
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
