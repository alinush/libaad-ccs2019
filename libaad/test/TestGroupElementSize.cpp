#include <aad/Configuration.h>

#include <fstream>
#include <cstdlib>
#include <boost/unordered_map.hpp>

#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>
#include <xutils/Log.h>

using namespace libaad;
using std::endl;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);
    srand(42);

    G1 g1 = G1::random_element();
    G2 g2 = G2::random_element();

    loginfo << "G1 needs " << g1.size_in_bits() << " bits" << endl;
    loginfo << "G2 needs " << g2.size_in_bits() << " bits" << endl;

    std::ofstream f1("/tmp/libff-group-g1-element-size");
    f1 << g1;
    loginfo << "G1 needs " << f1.tellp() << " bytes" << endl;
    f1.close();
    
    std::ofstream f2("/tmp/libff-group-g2-element-size");
    f2 << g2;
    loginfo << "G2 needs " << f2.tellp() << " bytes" << endl;
    f2.close();

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

