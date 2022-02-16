#include <gzstream.h>
#include <bzstream.h>
#include "fstream.h"
#include <iostream>
#include <stdlib.h>

using namespace compression;

template <typename comp_type>
void do_job(const char* filename)
{
    ofstream<comp_type>    out;
    out.open(filename);
    if ( ! out.good()) {
        std::cerr << "ERROR: Opening file " << filename  << "\n";
        return ;
    }
    for (int i = 0; i < 100; ++i) {
        out << "HELLO WORLD " << filename << " " << i << '\n' ;
    }
    out.close();
    ifstream<comp_type> in(filename);
    if ( ! in.good()) {
        std::cerr << "ERROR: Opening file failed " << filename << "\n";
        return;
    }
    char c;
    while (in.get(c)) {
        std::cout << c;
    }
}

int main()
{
    do_job<gz::streambuf>("test.gz");
    do_job<bz::streambuf>("test.bz");
}