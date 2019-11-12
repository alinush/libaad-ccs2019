#include <aad/Configuration.h>

#include <cstdlib>
#include <boost/unordered_map.hpp>

#include <aad/AADS.h>
#include <aad/BitString.h>
#include <aad/Hashing.h>
#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>

using namespace libaad;
using namespace libaad;
using std::endl;
using std::cout;

void testAppendsAndProofs(PublicParameters *pp, int n = 1024);
void simpleAadTest();
void testFrees(int n);

void printUsage(const char * prog) {
    cout << "Usage: " << prog << " [rand-seed] [num-appends] [public-params]" << endl;
    cout << endl;
    cout << "If public parameters are specified, then actually computes the authentication info for the data structure (slow)." << endl;
}

constexpr int SecParam = 128;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);
    std::string ppFile;
    std::unique_ptr<PublicParameters> pp;

    unsigned int seed = 42;
    int n = 16;
    if(argc > 1) {
        std::string arg = argv[1];
        if(arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else {
            seed = static_cast<unsigned int>(std::stoi(arg));
        }
    }
    if(argc > 2) {
        n = std::stoi(argv[2]);
    }
    if(argc > 3) {
        ppFile = argv[3];
        pp.reset(new PublicParameters(ppFile, n*SecParam*4, true, false));
    }
    loginfo << "Seeding PRNG with seed " << seed << endl;
    srand(seed);

    testFrees(7);
    
    Frontier frontier;
    BitString bs;
    bs << 0 << 1;
    frontier.addMissingKeyPrefix(bs);

    {
        //ScopedTimer<std::chrono::milliseconds> t(std::cout, "Appends took ");
        testAppendsAndProofs(pp.get(), n);
    }

    //std::cout << std::endl << std::endl;

    loginfo << endl;
    loginfo << "Doing a simple AAD test" << endl;

    simpleAadTest();

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void simpleAadTest() {
    AAD<std::string, std::string, true, SecParam> aad;
    const auto& forest = aad.getIndexedForest();
    
    auto checkAppend = [&forest, &aad](int numForestTrees, std::string key) {
        testAssertEqual(forest.getNumTrees(), numForestTrees);
        auto proof = aad.completeMembershipProof(key);
        //loginfo << "Proof for " << key << " (AAD size = " << aad.getSize() << ")" << endl << *proof << endl;
    };

    aad.append("k1", "v1.1");
    checkAppend(1, "k1");
    aad.append("k1", "v1.2");
    checkAppend(1, "k1");
    aad.append("k1", "v1.3");
    checkAppend(2, "k1");
    aad.append("k2", "v2.1");
    checkAppend(1, "k2");

    aad.append("k2", "v2.1"); // reinsert same key
    checkAppend(2, "k2");
    aad.append("k3", "v3.1");
    checkAppend(2, "k3");
    
    aad.append("k3", "v3.2");
    checkAppend(3, "k3");

    aad.append("k3", "v3.2"); // reinsert the same key
    checkAppend(1, "k3");
}

void testFrees(int n) {
    AAD<std::string, std::string> aad;
    for(int i = 0; i < n; i++) {
        std::string key = "k" + std::to_string(i+1);
        std::string value = key + "-val-" + std::to_string(i+1);
        loginfo << "Inserting key-value pair #" << i+1 << endl;
        aad.append(key, value);
    }
    loginfo << endl << "All done. Memory should be freed now..." << endl << endl;
}


void testAppendsAndProofs(PublicParameters *pp, int n) {
    AAD<std::string, std::string> aad(pp);
    size_t maxNumKeys = 3*static_cast<size_t>(n)/4;
    int prevPct = -1;
    bool progress = n >= 1024;
    for(int i = 0; i < n; i++) {
        // Pick a random key
        size_t r = static_cast<size_t>(rand()) % maxNumKeys;
        std::string key = "k" + std::to_string(r+1);
        std::string value = "v" + std::to_string(i+1);

        loginfo << endl;
        loginfo << "Inserting key-value pair #" << i+1 << ": (" << key << ", " << value << ")" << endl;

        aad.append(key, value);
        assertEqual(key, aad.getKeyByLeafNo(i));

        Digest digest = aad.getDigest();

        //logdbg << "Doing complete membership proofs for all keys inserted so far..." << endl;
        auto keys = aad.getKeys();
        for(auto& k : keys) {
            auto vals = aad.getValues(k);
            loginfo << "Proving membership of key " << k <<  " with " << vals.size() << " value(s)" << endl;
            auto proof = aad.completeMembershipProof(k);
            testAssertTrue(proof->verify(k, vals, digest));

            //int membProofSz = proof->getProofSize();
            //logdbg << "Membership proof size: " << Utils::humanizeBytes(membProofSz) << " (" << tup.second.size() << " values)" << endl;
        }
        
        //logdbg << "A few non-membership proofs..." << endl;
        for(int j = 0; j < i; j++) {
            int idx = i*n+j;
            std::string key = "n" + std::to_string(idx);
            loginfo << "Proving non-membership of key " << key << endl;
            auto proof = aad.completeMembershipProof(key);
            testAssertTrue(proof->verify(key, std::list<std::string>(), digest));

            //int nonMembProofSz = proof->getProofSize();
            //logdbg << "Non-membership proof size: " << Utils::humanizeBytes(nonMembProofSz) << endl;
        }

        int pct = static_cast<int>(static_cast<double>(i)/static_cast<double>(n) * 100.0);
        if(progress && pct > prevPct) {
            loginfo << pct << "% ... (i = " << i << " out of " << n << ")" << endl;
            prevPct = pct;
        }
    }
  
 
}
