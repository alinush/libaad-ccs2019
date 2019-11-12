#include <aad/Configuration.h>

#include <cstdlib>

#include <aad/AADS.h>
#include <aad/BitString.h>
#include <aad/Hashing.h>
#include <aad/Library.h>
#include <aad/Utils.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>

using namespace libaad;
using namespace libaad;
using std::endl;

void benchAppends(PublicParameters* pp, int n, int batchSize, bool sanityCheck, const std::string& fileName, bool progress = false);

constexpr int SecParam = 128;

int main(int argc, char *argv[])
{
    initialize(nullptr, 0);
    unsigned int seed = 42;
    srand(seed);
    bool sanityCheck = false;

    loginfo << "Initial NTL numThreads: " << NTL::AvailableThreads() << endl;

    int n = 8, batchSize = 1;
    if((argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) || argc < 2) {
        std::cout << "Usage: " << argv[0] << " <public-params-file> [n] [batchSize] [sanity-check] [ntlNumCores] [out-file]" << endl;
        std::cout << endl;
        std::cout << "Append times in <out-file> are in millseconds." << endl;
        return 1;
    }
    std::string ppFile = argv[1];
    if(argc > 2) {
        n = std::stoi(argv[2]);
    }

    if(argc > 3) {
        batchSize = std::stoi(argv[3]);
    }
    
    if(argc > 4) {
        sanityCheck = static_cast<bool>(std::stoi(argv[4]));
    }

    if(argc > 5) {
        int ntlNumCores = std::stoi(argv[5]);
        if(ntlNumCores > static_cast<int>(getNumCores())) {
            logerror << "Number of cores for libntl (" << ntlNumCores << ") cannot be bigger than # of cores on machine, which is " << getNumCores() << endl;
            return 1;
        }

        if(ntlNumCores > 0) {
            NTL::SetNumThreads(static_cast<long>(ntlNumCores));
            loginfo << "Changed NTL NumThreads to " << ntlNumCores << endl;
            loginfo << "NTL pool active: " << NTL::GetThreadPool()->active() << endl;
        }
    }

    std::string outFile = "aad-append-time-n-" + std::to_string(n) + "-b-" + std::to_string(batchSize) + ".csv";
    if(argc > 6) {
        outFile = argv[6];
    }


    std::unique_ptr<PublicParameters> pp(new PublicParameters(ppFile, n*SecParam*4, true, false));

    loginfo << "Randomness seed is " << seed << endl;
    loginfo << "AAD batch size is " << batchSize << endl;
    loginfo << "Sanity check: " << sanityCheck << endl;
    loginfo << "Benchmarking creating real AAD of " << n << " leafs" << endl;
    loginfo << "Outputting append times (in millisecs) in file '" << outFile << "'" << endl;

    {
        ScopedTimer<std::chrono::seconds> t(std::cout, "Appends took ", " seconds\n");
        benchAppends(pp.get(), n, batchSize, sanityCheck, outFile, true);
        std::cout << endl;
    }

    std::cout << endl;
    
    printMemUsage("Memory usage before exiting"); 

    std::cout << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void benchAppends(PublicParameters* pp, int n, int batchSize, bool sanityCheckEnabled, const std::string& fileName, bool progress) {
    // generate random keys with repeats
    std::vector<std::string> keys;
    keys.push_back("k" + std::to_string(1));
    for(int i = 0; i < 3*n/4; i++) {
        keys.push_back("k" + std::to_string(i+1));
    }

    // check we have enough q-SDH parameters
    if(SecParam * 4 * static_cast<size_t>(n) > pp->q+1) {
        logerror << "Need more than " << pp->q << " public params for n = " << n << " key value pairs" << endl;
        throw std::runtime_error("Not enough public params");
    }
    
    // random device for picking random leaves
    std::random_device rand_dev;
    std::mt19937       generator(rand_dev());
    
    // output append times to this file
    ofstream fout(fileName);
    fout << "dictSize,batchSize,appendTimeMillisec," << endl;
 
    AAD<std::string, std::string, true, SecParam> aad(pp);
    aad.setBatchSize(batchSize);

    AveragingTimer t("");
    auto append = [batchSize, &aad, &keys, &fout, &t](int i) {
        // Pick a random key
        size_t r = static_cast<size_t>(rand()) % keys.size();
        //loginfo << endl;
        //loginfo << "Appending entry #" << i+1 << " out of " << n << endl;

        // append key
        t.startLap();
        aad.append(keys[r], "v" + std::to_string(i+1));
        auto usec = t.endLap();

        // record time
        fout << i + 1 << ", ";
        fout << batchSize << ", ";
        fout << usec / 1000 << ", ";
        fout << endl;
    };

    int prevPct = -1;
    auto progressBar = [progress, &prevPct, n](int i) {
        // display progress
        if(progress) {
            int pct = static_cast<int>(static_cast<double>(i)/static_cast<double>(n) * 100.0);
            if(pct > prevPct) {
                loginfo << endl;
                loginfo << pct << "% ... (i = " << i << " out of " << n << ")" << endl;
                prevPct = pct;
            }
        }
    };

    auto sanityCheck = [sanityCheckEnabled, &aad, &generator](int maxLeaf) {
        if(!sanityCheckEnabled)
            return;

        // sanity check: do a membership and non-membership proof
        std::uniform_int_distribution<int>  distr(0, maxLeaf);
        int leaf = distr(generator);
        auto key = aad.getKeyByLeafNo(leaf);
        auto noKey = "n" + std::to_string(leaf);
        Digest digest = aad.getDigest();

        loginfo << "Doing memb proof for '" << key << "' and non-membership proof for '" << noKey << "'" << endl;

        auto mp = aad.completeMembershipProof(key);
        std::list<std::string> vals = aad.getValues(key);
        testAssertTrue(mp->verify(key, vals, digest));

        auto nmp = aad.completeMembershipProof(noKey);
        testAssertTrue(nmp->verify(noKey, std::list<std::string>(), digest));
    };

    int count = 0;
    int numBatches = n / batchSize;
    for(int i = 0; i < numBatches; i++) {
        loginfo << "Adding batch " << i << " out of " << numBatches << endl;
        for(int j = 0; j < batchSize; j++) {
            append(count++);
            progressBar(count);
        }
        
        sanityCheck(count - 1);
    }

    int leftOver = n % batchSize;
    if(leftOver > 0) {
        for(int i = 0; i < leftOver; i++) {
            append(count++);
            progressBar(count);
        }
        
        sanityCheck(count - 1);
    }

    loginfo << "Finished appending " << n << " key value pairs (batchSize = " << batchSize << ")" << endl << endl;

    logperf << "Average time per append: " << t.averageLapTime() / 1000 << " milliseconds" << endl << endl;
    
    printMemUsage("Memory usage with full AAD before exiting"); 
}
