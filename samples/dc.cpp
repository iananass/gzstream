#include <gzstream.h>
#include <bzstream.h>
#include "fstream.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

using namespace compression;

template <typename comp_type>
int decompress(const char* in_filename, const char* out_filename)
{
    ifstream<comp_type> in(in_filename);
    if (!in.good()) {
        std::cerr << "ERROR: Opening file `" << in_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    std::ofstream out(out_filename);
    if (!out.good()) {
        std::cerr << "ERROR: Opening file `" << out_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    char c;
    while (in.get(c)) {
        out << c;
    }
    in.close();
    out.close();
    if (!in.eof()) {
        std::cerr << "ERROR: Reading file `" << in_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    if (!out.good()) {
        std::cerr << "ERROR: Writing file `" << out_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

template <typename comp_type>
int compress(const char* in_filename, const char* out_filename)
{
    std::ifstream in(in_filename);
    if (!in.good()) {
        std::cerr << "ERROR: Opening file `" << in_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    ofstream<comp_type> out(out_filename);
    if (!out.good()) {
        std::cerr << "ERROR: Opening file `" << out_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    char c;
    while (in.get(c))
        out << c;
    in.close();
    out.close();
    if (!in.eof()) {
        std::cerr << "ERROR: Reading file `" << in_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    if (!out.good()) {
        std::cerr << "ERROR: Writing file `" << out_filename << "' failed.\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

typedef int (*action)(const char*, const char*);

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <c|d> <bz|gz> <in-file> <out-file>\n";
        return EXIT_FAILURE;
    }
    action act = nullptr;
    if (strcmp(argv[2], "bz") == 0 ) {
        if (argv[1][0] == 'c') {
            act = compress<bz::streambuf>;
        } else if (argv[1][0] == 'd') {
            act = decompress<bz::streambuf>;
        } else {
            std::cerr << "Unknown action `" << argv[1] << "`\n";
            return EXIT_FAILURE;
        }
    } else  if (strcmp(argv[2], "gz") == 0 ) {
        if (argv[1][0] == 'c') {
            act = compress<gz::streambuf>;
        } else if (argv[1][0] == 'd') {
            act = decompress<gz::streambuf>;
        } else {
            std::cerr << "Unknown action `" << argv[1] << "`\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cerr << "Unknown algorithm `" << argv[2] << "`\n";
        return EXIT_FAILURE;
    }

    act(argv[3], argv[4]);

}