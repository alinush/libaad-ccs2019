#include <aad/Configuration.h>

#include <cstdlib>
#include <deque>

#include <aad/AADS.h>
#include <aad/BitString.h>
#include <aad/Hashing.h>
#include <aad/Library.h>
#include <aad/Utils.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>

using namespace libaad;
using std::endl;

/**
 * Output format:
 * oldDictSize, newDictSize, proofBytes, verifyUsec,
 */

void benchAppendOnlyProofSize(std::deque<int>& oldDictSizes, std::deque<int>& newDictSizes, const std::string& fileName, bool progress);

int main(int argc, char *argv[])
{
    std::string fileName = "aad-append-only-proof.csv";

    initialize(nullptr, 0);
    srand(42);

    /**
     * Target AAD sizes for the measuring append-only proof size specified as tuples:
     * <old-size-1> <old-size-2> [<old-size-i> ...] <new-size-1> <new-size-2> [<new-size-i> ...]
     *
     * Measures append-only proof between <old-size-i> and <new-size-i>.
     * Example: 1 3 7 15    3 7 15 31
     * Will measure proof between (1, 3), (3,7), (7,15), (15, 31)
     * Another example: 10 100 1000              100 1000 10000
     * Another example: 1 3 7 15 10 100 1000     3 7 15 31 100 1000 10000
     * Another example: 1 3 1 7 15 10            3 7 10 15 31 100
     * (Note: new sizes must be sorted, but old sizes need not be sorted!)
     */
    std::deque<int> oldDictSizes, newDictSizes;
    if(argc > 1) {
        int sizes = argc - 1;
        if(sizes % 2 != 0) {
            throw std::runtime_error("Need an even number of old sizes and new sizes");
        }
        
        for(int i = 0; i < sizes / 2; i++) {
            int oldSize = std::stoi(argv[i + 1]);
            oldDictSizes.push_back(oldSize);

            int newSize = std::stoi(argv[sizes / 2 + i + 1]);
            newDictSizes.push_back(newSize);
        }
    } else {
        oldDictSizes.push_back(31);
        oldDictSizes.push_back(63);
        oldDictSizes.push_back(127);

        newDictSizes.push_back(63);
        newDictSizes.push_back(127);
        newDictSizes.push_back(255);
    }
    
    if(!std::is_sorted(newDictSizes.begin(), newDictSizes.end())) {
        std::cout << endl;
        logerror << "-----------------------------------------------" << endl;
        logerror << "You're supposed to have the new sizes sorted!  " << endl;
        logerror << "(with matching old sizes in the right position)" << endl;
        logerror << "-----------------------------------------------" << endl;
        std::cout << endl;
        logerror << "Exiting due to error..." << endl;
        return 1;
    }
    
    loginfo << "Benchmarking append-only proof sizes between AADs of size: " << endl;
    for(size_t i = 0; i < oldDictSizes.size(); i++) {
        loginfo << "(" << oldDictSizes[i] << ", " << newDictSizes[i] << ")" << endl;
    }

    benchAppendOnlyProofSize(oldDictSizes, newDictSizes, fileName, true);

    printMemUsage("Memory usage before exiting benchmark");
    loginfo << endl;
    std::cout << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void benchAppendOnlyProofSize(std::deque<int>& oldDictSizes, std::deque<int>& newDictSizes, const std::string& fileName, bool progress) {
    testAssertEqual(oldDictSizes.size(), newDictSizes.size());

    ofstream fout(fileName);
    fout << "oldDictSize,newDictSize,proofBytes,verifyUsec" << endl;

    int prevPct = -1;
    int keyNo = 0;
    int maxSize = newDictSizes.back();
    AAD<std::string, std::string, false> aad;

    while(!newDictSizes.empty()) {
        int oldSize = oldDictSizes.front();
        int newSize = newDictSizes.front();
        oldDictSizes.pop_front();
        newDictSizes.pop_front();
            
        fout << oldSize << ", " << newSize << ", " << std::flush;

        while(aad.getSize() < newSize) {
            // Append dummy key-value pair
            auto key = "k" + std::to_string(keyNo);
            auto val = "v" + std::to_string(keyNo);
            aad.append(key, val);

            int pct = static_cast<int>(static_cast<double>(aad.getSize())/static_cast<double>(maxSize) * 100.0);
            if(progress && pct > prevPct) {
                loginfo << pct << "% ... (i = " << aad.getSize() << " out of " << maxSize << ")" << endl;
                prevPct = pct;

                printMemUsage();
                loginfo << endl;
            }
        }

        testAssertEqual(aad.getSize(), newSize);
        
        auto aop = aad.appendOnlyProof(oldSize);
        int proofSize = aop->getProofSize();
        fout << proofSize << ", " << std::flush;

        // time proof verification
        ManualTimer t;
        testAssertTrue(aop->verify(aad.getDigest(oldSize), aad.getDigest()));
        auto verTime = t.stop().count();
        
        fout << verTime << std::flush;
        fout << std::endl;
    
        logperf << "Append-only proof verification time " << verTime << " microseconds" << endl;
    }
}
