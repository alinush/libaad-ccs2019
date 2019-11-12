#include <aad/Configuration.h>

#include <cstdlib>

#include <aad/AADS.h>
#include <aad/BitString.h>
#include <aad/Hashing.h>
#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>

using namespace libaad;
using namespace libaad;
using std::endl;

void benchFrontierSizes(int n, bool progress = false);

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);
    srand(42);

    int n = 1024*16-1;
    if(argc > 1) {
        n = std::stoi(argv[1]);
    }

    loginfo << endl;
    loginfo << "---------------------------------------------------------------------------------" << endl;
    loginfo << "WARNING: Frontier sizes refer to all nodes in the frontier tree, not just leaves!" << endl;
    loginfo << "---------------------------------------------------------------------------------" << endl;
    loginfo << endl;
    loginfo << "Benchmarking frontier sizes for AAD of " << n << " leafs" << endl;
    benchFrontierSizes(n, true);

    std::cout << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

template<class AAD>
void printRootSizes(const AAD& aad, int currSize) {
    auto roots = aad.getRootATs();
    loginfo << "AccTree sizes:  ";
    long totalSize = 0;
    for(auto root : roots) {
        auto at = std::get<0>(root);
        
        int size = at->getSize();
        totalSize += size;
        cout << size << ", ";
    }
    cout << "(" << totalSize/currSize << "x overhead)" << endl;

    loginfo << "Frontier sizes: ";
    totalSize = 0;
    for(auto root : roots) {
        auto frontier = std::get<1>(root);
        
        int size = frontier->getSize();
        totalSize += size;
        cout << size  << ", ";
    }
    cout << "(" << totalSize/currSize << "x overhead)" << endl;
    cout << endl;
}

void benchFrontierSizes(int n, bool progress) {
    std::vector<std::string> keys;
    keys.push_back("k" + std::to_string(1));
    for(int i = 0; i < 3*n/4; i++) {
        keys.push_back("k" + std::to_string(i+1));
    }

    AAD<std::string, std::string> aad;
    int prevPct = -1;
    for(int i = 0; i < n; i++) {
        // Pick a random key
        size_t r = static_cast<size_t>(rand()) % keys.size();
        aad.append(keys[r], "v" + std::to_string(i+1));
        
        if(progress) {
            int pct = static_cast<int>(static_cast<double>(i)/static_cast<double>(n) * 100.0);
            if(pct > prevPct) {
                loginfo << pct << "% ... (i = " << i << " out of " << n << ")" << endl;
                prevPct = pct;

                printRootSizes(aad, i+1);
            }
        }
    }
    loginfo << "Finished appending " << n << " key value pairs" << endl;

    loginfo << "Final sizes for n = " << n << endl;
    printRootSizes(aad, n);
}
