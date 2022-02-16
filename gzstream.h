// ============================================================================
// gzstream, C++ iostream classes wrapping the zlib compression library.
// Copyright (C) 2001  Deepak Bandyopadhyay, Lutz Kettner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// ============================================================================
//
// File          : gzstream.h
// Revision      : $Revision: 1.5 $
// Revision_date : $Date: 2002/04/26 23:30:15 $
// Author(s)     : Deepak Bandyopadhyay, Lutz Kettner
// 
// Standard streambuf implementation following Nicolai Josuttis, "The 
// Standard C++ Library".
// ============================================================================

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

class streambase : virtual public std::ios
{
protected:
    streambuf buf;
public:
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
};

// ----------------------------------------------------------------------------
// User classes. Use ifstream and ofstream analogously to ifstream and
// ofstream respectively. They read and write files based on the gz*
// function interface of the zlib. Files are compatible with gzip compression.
// ----------------------------------------------------------------------------

class ifstream : public streambase, public std::istream
{
public:
    ifstream() : std::istream(&buf)
    {}

    ifstream(const char* name, int open_mode = std::ios::in)
            : streambase(name, open_mode), std::istream(&buf)
    {}

    streambuf* rdbuf()
    { return streambase::rdbuf(); }

    void open(const char* name, int open_mode = std::ios::in)
    {
        streambase::open(name, open_mode);
    }
};

class ofstream : public streambase, public std::ostream
{
public:
    ofstream() : std::ostream(&buf)
    {}

    ofstream(const char* name, int mode = std::ios::out)
            : streambase(name, mode), std::ostream(&buf)
    {}

    streambuf* rdbuf()
    { return streambase::rdbuf(); }

    void open(const char* name, int open_mode = std::ios::out)
    {
        streambase::open(name, open_mode);
    }
};

} // gz


