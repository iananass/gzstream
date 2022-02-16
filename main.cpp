#include "gzstream.h"
#include <iostream>
#include <stdlib.h>

int main() {
    gz::ofstream    out;
    out.open("test.z");
    if ( ! out.good()) {
        std::cerr << "ERROR: Opening file\n";
        return EXIT_FAILURE;
    }
    for (int i = 0; i < 100; ++i) {
        out << "HELLO WORLD" << i << '\n' ;
    }
    out.close();
    gz::ifstream in("test.z");
    if ( ! in.good()) {
        std::cerr << "ERROR: Opening file failed.\n";
        return EXIT_FAILURE;
    }
    char c;
    while (in.get(c)) {
        std::cout << c;
    }
}